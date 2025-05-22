// 2008-06-05
// _XLog.cpp
// guoshanhe
// ��־�ࣨ���̰߳�ȫ��

#include "XLog.h"
#include "XThread.h"
#include "XTime.h"
#include "XCritical.h"
#include "XEvent.h"
#include "XFileUtil.h"
#include "XStrUtil.h"
#include "XSocket.h"

namespace xbase {

#define X_DEFAULT_MAX_LINE_EACH_LOG_FILE (32 * 1024)
#define X_DEFAULT_LOG_MSG_MAX_LENGTH	1024
#define X_DEFAULT_MAX_MALLOC_BUFFER		(32 * 1024)

static const char *gs_caszPriorityString[XLOG_PRIORITY_MAX] =
{
	"NONE ",
	"CRASH",
	"ERROR",
	"WARNI",
	"NOTIC",
	"INFO ",
	"DEBUG",
	"TRACE"
};

////////////////////////////////////////////////////////////////////////////////
// _XLog
////////////////////////////////////////////////////////////////////////////////
// ������־��
class _XLog : public XThread
{
public:
	// ��ȡ����Ψһʵ��
	static _XLog *Instance()
	{
		static _XLog instance;
		return &instance;
	}

	// ��־ϵͳ��ʼ��
	BOOL Init(const XLogParam& logParam);
	BOOL UnInit(void);

	//  ������־�������Ϣ����
	void SetPriority(uint32 uPriority/*see XLogPriority*/);
	uint32 GetPriority() { return m_uPriority; }

	// ������־���λ��
	void SetOptions(uint32 uOptions/*see XLOG_OPTIONS*/);
	uint32 GetOptions() { return m_uOptions; }

	// ������־��������
	void SetKeepDays(uint32 uDays);
	uint32 GetKeepDays() { return m_uKeepDays; }

	// ����tcp��־�������˿�
	BOOL SetTcpPort(uint32 port);

	// ���
	BOOL Printf(uint32 nPriority/*see XLogPriority*/, const char cszFormat[], va_list marker);

private:
	_XLog();
	virtual ~_XLog();

	// ��ʼ��Tcp����
	BOOL _InitAcceptor();

	// ����tcp����
	void _AcceptTcpStream();

	// ʵ��д(len������'\0'����)
	BOOL _RealWrite(const char *pszBuffer, uint32 len);

	// д��log�ļ�(len������'\0'����)
	BOOL _WriteFile(const char *pszBuffer, uint32 len);

	// д��tcp��(len������'\0'����)
	void _WriteTcpStream(const char *pszBuffer, uint32 len);

	// ������־�ļ�
	BOOL _ChangeLogFile(void);

	// ������־�ļ�(����ɾ��)
	void _RemoveOldLogFile();

protected:
	virtual void Entry();

private:
	uint32				m_uOptions;/*see XLOG_OPTIONS, join use '|'*/
	uint32				m_uPriority;

	string				m_strPath;
	string				m_strIdent;
	uint32				m_uKeepDays;
	uint32				m_uMaxLineEachFile;
	uint32				m_uCurrentLine;
	string				m_strCurrFileName;
	FILE *				m_fdLogFile;

	XCritical			m_lockFree;
	list<void *>		m_lstFreeBufs;
	XCritical			m_lockUse;
	list<void *>		m_lstUseBufs;
	XEvent				m_evWork;

	XSocket				m_listensock;
	uint32				m_listenport;
	list<SOCKET>		m_lstsockets;

	BOOL				m_bInitOK;
};

_XLog::_XLog()
	: m_uOptions(XLOG_OPTION_CONSOLE)
	, m_uPriority(XLOG_DEBUG)
	, m_uKeepDays(30)
	, m_uMaxLineEachFile(0)
	, m_uCurrentLine(0)
	, m_fdLogFile(NULL)
	, m_listenport(0)
	, m_bInitOK(FALSE)
{
	m_lstFreeBufs.clear();
	m_lstUseBufs.clear();
	m_lstsockets.clear();
}

_XLog::~_XLog()
{
	UnInit();
}

// ��ʼ��
BOOL _XLog::Init(const XLogParam& logParam)
{
	if (m_bInitOK)
	{
		fprintf(stderr, "ERR: Log system re-init!\n");
		return FALSE;
	}

	m_uOptions = logParam.uOptions;
	if (m_uOptions == 0)
	{
		m_uOptions = XLOG_OPTION_CONSOLE;
	}

	m_uPriority = logParam.uPriority;
	if (m_uPriority <= 0 || m_uPriority >= XLOG_PRIORITY_MAX)
	{
		m_uPriority = XLOG_DEBUG;
	}
	
	m_uMaxLineEachFile = logParam.uMaxLineEachFile;
	if (m_uMaxLineEachFile == 0 || m_uMaxLineEachFile > X_DEFAULT_MAX_LINE_EACH_LOG_FILE)
	{
		m_uMaxLineEachFile = X_DEFAULT_MAX_LINE_EACH_LOG_FILE;
	}
	else if (m_uMaxLineEachFile <= 1024)
	{
		m_uMaxLineEachFile = 1024;
	}
	m_uCurrentLine = m_uMaxLineEachFile; // ʹ��һ��д��־ʱ�ʹ����ļ���

	m_uKeepDays = logParam.uKeepDays;

	if (m_uOptions & XLOG_OPTION_FILE)
	{
		m_strPath = logParam.strPath;
		m_strIdent = logParam.strIdent;
		XStrUtil::Chop(m_strPath, " \r\n\t");
		if (m_strPath.size() > 1) XStrUtil::ChopTail(m_strPath, "\\/");
		XStrUtil::Chop(m_strIdent, "\r\n\t\\/");
		if (m_strPath.empty() || m_strIdent.empty())
		{
			fprintf(stderr, "ERR: Log path or ident is empty!\n");
			return FALSE;
		}
	}

	if (m_uOptions & XLOG_OPTION_TCP)
	{
		m_listenport = logParam.uListenPort;
		if (!_InitAcceptor())
		{
			fprintf(stderr, "ERR: Log system init tcp listen(port:%d) failed!\n", m_listenport);
			return FALSE;
		}
	}

	if (!Start())	// �����߳�
	{
		fprintf(stderr, "ERR: Log system thread start failed!\n");
		return FALSE;
	}

	m_bInitOK = TRUE;
	return TRUE;
}

BOOL _XLog::UnInit()
{
	if (!m_bInitOK)
	{
		return TRUE;
	}

	// д��ʣ����־
	while (true)
		{
			m_lockUse.Lock();
		uint32 count = (uint32)m_lstUseBufs.size();
			m_lockUse.UnLock();
			if (count == 0) break;
			m_evWork.Set();
		X_Sleep(100);
		}

	// ֹͣ�߳�
	PostStop();
		m_evWork.Set();
	Join();

	// ���ļ����
	if (m_fdLogFile)
	{
		fclose(m_fdLogFile);
		m_fdLogFile= NULL;
	}

	// ��tcp��
	m_listensock.Close();
	while (m_lstsockets.size() > 0)
	{
		SOCKET s = m_lstsockets.front();
		m_lstsockets.pop_front();
		XSocket sock(s);
	}

	// �ͷ�����������ڴ�
	while (m_lstFreeBufs.size() > 0)
	{
		delete (char *)m_lstFreeBufs.front();
		m_lstFreeBufs.pop_front();
	}
	
	m_bInitOK = FALSE;
	return TRUE;
}

void _XLog::SetOptions(uint32 uOptions/*see XLOG_OPTIONS*/)
{
	if ((uOptions & 0X0F) == 0) return;
	m_uOptions = (uOptions & 0X0F);
}

void _XLog::SetPriority(uint32 uPriority)
{
	m_uPriority = uPriority;
	if (m_uPriority < XLOG_NONE)
	{
		m_uPriority = XLOG_NONE;
	}
	if (m_uPriority >= XLOG_PRIORITY_MAX)
	{
		m_uPriority = XLOG_PRIORITY_MAX - 1;
	}
}

void _XLog::SetKeepDays(uint32 uDays)
{
	m_uKeepDays = uDays;
	if (m_uKeepDays > 365) m_uKeepDays = 365;
	_RemoveOldLogFile();
	return;
}

BOOL _XLog::SetTcpPort(uint32 port)
{
	if (port == 0)
	{
		m_uOptions &= ~XLOG_OPTION_TCP;
		m_listenport = 0;
		m_listensock.Close();
		while (m_lstsockets.size() > 0)
		{
			SOCKET s = m_lstsockets.front();
			m_lstsockets.pop_front();
			XSocket sock(s);
		}
		return TRUE;
	}
	else
	{
		m_uOptions |= XLOG_OPTION_TCP;
		m_listenport = port;
		return _InitAcceptor();
	}
	return FALSE;
}

// ���
BOOL _XLog::Printf(uint32 uPriority, const char cszFormat[], va_list marker)
{
	int nRetCode = -1;
	XTime now;
	uint32 nStrLen = 0;
	uint32 nBufferLen  = 0;
	char *pszBuffer = NULL;
	char *pvData = NULL;

	if (!m_bInitOK) return FALSE;
	if (cszFormat == NULL) return FALSE;
	if (uPriority == XLOG_NONE) return TRUE;
	if (uPriority > m_uPriority) return TRUE;

	// �����

	m_lockUse.Lock();
	if (m_lstUseBufs.size() > X_DEFAULT_MAX_MALLOC_BUFFER){
		m_lockUse.UnLock();
		return FALSE;
	}
	m_lockUse.UnLock();

	// �����ڴ�
	m_lockFree.Lock();
	if (m_lstFreeBufs.size() > 0)
	{
		pvData = (char *)m_lstFreeBufs.front();
		m_lstFreeBufs.pop_front();
	}
	else
	{
		pvData = new(std::nothrow)char[X_DEFAULT_LOG_MSG_MAX_LENGTH];
	}
	m_lockFree.UnLock();
	if (pvData == NULL)
	{
		fprintf(stderr, "Log system malloc failed!\n");
		return FALSE;
	}

	pszBuffer = (char *)pvData + sizeof(uint32);
	nBufferLen = (uint32)(X_DEFAULT_LOG_MSG_MAX_LENGTH - sizeof(uint32));
	nRetCode = snprintf(pszBuffer, nBufferLen - 1,
						"%04d%02d%02d-%02d:%02d:%02d<%s:%u>: ",
						now.local_year(),
						now.local_mon(),
						now.local_mday(),
						now.local_hour(),
						now.local_min(),
						now.local_sec(),
						gs_caszPriorityString[uPriority],
						X_GetThreadID()
						);
	if (nRetCode <= 0)
	{
		XLockGuard<XCritical> lock(m_lockFree);
		m_lstFreeBufs.push_back(pvData);
		return FALSE;
	}
	nStrLen = nRetCode;

	nRetCode = vsnprintf(
		pszBuffer + nStrLen, 
		nBufferLen - nStrLen - 1,
		cszFormat, marker
		);
	if (nRetCode < 0 || nRetCode >= (int)(nBufferLen - nStrLen - 1))
		nStrLen = nBufferLen - 1;	// windows�����ַ�����������ʱ,�᷵��-1. linuw�»᷵��ֵ���ܴ��ڻ�������С.���ǻ��������ᱻ���.
	else
		nStrLen += nRetCode;

	while (pszBuffer[nStrLen - 1] == '\r' || pszBuffer[nStrLen - 1] == '\n')
	{
		nStrLen--;
	}
	if (nStrLen >= nBufferLen - 2)	// if full
	{
		nStrLen = nBufferLen - 3;
	}
	pszBuffer[nStrLen] = '\r';
	pszBuffer[nStrLen + 1] = '\n';
	pszBuffer[nStrLen + 2] = '\0';
	nStrLen += 2;

	// ʹ�ö�����߳�д
	XLockGuard<XCritical> lock(m_lockUse);
	*((uint32 *)pvData) = (uint32)nStrLen;
	m_lstUseBufs.push_back(pvData);
	m_evWork.Set();	// ���¼�

	return TRUE;
}

void _XLog::Entry()
{
	void *pvData = NULL;
	char *pszBuffer = NULL;
	uint32 len = 0;

	while (!TryWaitQuit())
	{
		pvData = NULL;
		m_lockUse.Lock();
		if (m_lstUseBufs.size() > 0)
		{
			pvData = m_lstUseBufs.front();
			m_lstUseBufs.pop_front();
		}
		m_lockUse.UnLock();
		if (pvData == NULL)
		{
			// ����ʱ����TCP����
			if (m_uOptions & XLOG_OPTION_TCP)
			{
				_AcceptTcpStream();
			}

			// ����ʱ�ȴ����500����
			m_evWork.TryWait(500);

			continue;
		}

		pszBuffer = (char *)pvData + sizeof(uint32);
		len = *(uint32 *)pvData;

		// ʵ��д
		_RealWrite(pszBuffer, len);

		// �Żص��ڴ滺����
		m_lockFree.Lock();
		m_lstFreeBufs.push_back(pvData);
		m_lockFree.UnLock();
	}

	return;
}

// ��ʼ��Tcp����
BOOL _XLog::_InitAcceptor()
{
	if (m_listenport == 0) return FALSE;

	m_listensock.Close();
	if (!m_listensock.Open(SOCK_STREAM))
	{
		return FALSE;
	}
	m_listensock.SetReUseAddr();
	if (!m_listensock.Listen("", m_listenport))
	{
		m_listensock.Close();
		return FALSE;
	}

	if (!m_listensock.SetNonBlock())
	{
		m_listensock.Close();
		return FALSE;
	}

	return TRUE;
}

// ����tcp����
void _XLog::_AcceptTcpStream()
{
	XSocket sock;
	if (!m_listensock.Accept(sock))
	{
		_InitAcceptor();
		return;
	}
	if (sock.GetHandle() == INVALID_SOCKET)
	{
		return;
	}
	if (!sock.SetNonBlock())
	{
		sock.Close();
		return;
	}

	string addr;
	uint16 port = 0;
	sock.GetRemoteAddr(addr, port);
	XLog::Printf(XLOG_NOTICE, "Log system tcp accept a socket(%s:%d).", addr.c_str(), port);

	if (m_lstsockets.size() >= 5) // ��������������
	{
		sock.Send("Server busy!!!\r\n", 16);
		XLog::Printf(XLOG_NOTICE, "Log system tcp connection count > 5, new socket disconnect.");
		sock.Close();
		return;
	}

	m_lstsockets.push_back(sock.Detach());
	return;
}

// ʵ��д(len������'\0'����)
BOOL _XLog::_RealWrite(const char *pszBuffer, uint32 len)
{
	if (m_uOptions & XLOG_OPTION_CONSOLE)
	{
		fputs(pszBuffer, stdout);
	}
	if (m_uOptions & XLOG_OPTION_STDERR)
	{
		fputs(pszBuffer, stderr);
	}
	if (m_uOptions & XLOG_OPTION_FILE)
	{
		_WriteFile(pszBuffer, len);
	}
	if (m_uOptions & XLOG_OPTION_TCP)
	{
		_WriteTcpStream(pszBuffer, len);
	}
	return TRUE;
}

// д��log�ļ�(len������'\0'����)
BOOL _XLog::_WriteFile(const char *pszBuffer, uint32 len)
{
	int nRetCode = -1;

	if (m_uCurrentLine >= m_uMaxLineEachFile || m_fdLogFile == NULL)
	{            
		nRetCode = _ChangeLogFile();
		if (!nRetCode) return FALSE;
		m_uCurrentLine = 0;
	}

	if (m_fdLogFile)
	{
#ifdef __WINDOWS__
		nRetCode = (int)fwrite(pszBuffer, 1, (int)len, m_fdLogFile);
#endif//__WINDOWS__
#ifdef __GNUC__
		nRetCode = write(fileno(m_fdLogFile), pszBuffer, (int)len);
#endif//__GNUC__
		if (nRetCode != (int)len)
		{
			fprintf(stderr, "Log system write file(%s) failed, errno:%d(%s)!\n", m_strCurrFileName.c_str(), errno, strerror(errno));
			return FALSE;
		}
		++m_uCurrentLine;
	}
	
	return TRUE;
}

// д��tcp��(len������'\0'����)
void _XLog::_WriteTcpStream(const char *pszBuffer, uint32 len)
{
	list<SOCKET>::iterator it;
	it = m_lstsockets.begin();
	while (it != m_lstsockets.end())
	{
		XSocket sock(*it);
		if (-1 == sock.Send(pszBuffer, len))
		{
			it = m_lstsockets.erase(it);

			string addr;
			uint16 port = 0;
			sock.GetRemoteAddr(addr, port);
			XLog::Printf(XLOG_NOTICE, "Log system tcp socket disconnect(%s:%d).", addr.c_str(), port);
			sock.Close();
		}
		else
		{
			sock.Detach();
			++it;
		}
	}
	return;
}

// ������־�ļ�
BOOL _XLog::_ChangeLogFile(void)
{
	BOOL bRet = FALSE;
	int nRetCode = -1;
	XTime now;
	char szLogCurrDirPath[PATH_MAX] = {};
	char szLogCurrFilePath[PATH_MAX] = {};
	char szLogFileName[256] = {};

	// make dir path
	nRetCode = snprintf(
		szLogCurrDirPath, sizeof(szLogCurrDirPath) - 1,
		"%s%c%04d%02d%02d", 
		m_strPath.c_str(),
		X_PATH_SEPARATOR_CHAR,
		now.local_year(),
		now.local_mon(),
		now.local_mday()
		);
	if (nRetCode <= 0) return FALSE;

	bRet = XFileUtil::MakeFullDir(szLogCurrDirPath);
	if (!bRet) return FALSE;

	// make file name
	nRetCode = snprintf(
		szLogFileName, sizeof(szLogFileName) - 1,
		"%s_%04d%02d%02d%02d%02d%02d.log", 
		m_strIdent.c_str(),
		now.local_year(),
		now.local_mon(),
		now.local_mday(),
		now.local_hour(),
		now.local_min(),
		now.local_sec()
		);
	if (!bRet) return FALSE;

	// make file path
	nRetCode = snprintf(
		szLogCurrFilePath, sizeof(szLogCurrFilePath) - 1,
		"%s%c%s", 
		szLogCurrDirPath,
		X_PATH_SEPARATOR_CHAR,
		szLogFileName
		);
	if (!bRet) return FALSE;

	if (m_fdLogFile)
	{
		fclose(m_fdLogFile);
		m_fdLogFile = NULL;
		m_strCurrFileName.clear();
	}

	m_fdLogFile = fopen(szLogCurrFilePath, "at+");
	if (m_fdLogFile == NULL)
	{
		fprintf(stderr, "Log system open file(%s) failed, errno:%d(%s)!\n", szLogCurrFilePath, errno, strerror(errno));
		return FALSE;
	}
	m_strCurrFileName = szLogFileName;
	
	// ��ʹ�û���
	setvbuf(m_fdLogFile, NULL, 0, _IONBF);

	// remove old file
	_RemoveOldLogFile();

	return TRUE;
}

void _XLog::_RemoveOldLogFile()
{
	time_t last = time(NULL) - 3600 * 24 * m_uKeepDays;
	XFileFinder finder;
	string strName;
	string strDirName;
	string strFileName;
	int type;

	if (!finder.FindFirst(m_strPath, XFileFinder::X_TYPE_DIR)) return;
	while ((type = finder.FindNext(strName) != XFileFinder::X_TYPE_NONE))
	{
		if (!(type & XFileFinder::X_TYPE_DIR))
		{
			strFileName = m_strPath + X_PATH_SEPARATOR_CHAR + strName;
			XFileUtil::RemoveFile(strFileName);
		}
		else
		{
			XFileFinder subFinder;
			strDirName = m_strPath + X_PATH_SEPARATOR_CHAR + strName;
			if (!subFinder.FindFirst(strDirName)) continue;
			while ((type = subFinder.FindNext(strName) != XFileFinder::X_TYPE_NONE))
			{
				if (strName == m_strCurrFileName) continue;
				size_t begin = strName.find('_', 0);
				size_t end = strName.rfind('.');
				if (begin != string::npos && end > begin)
				{
					string strTmp = strName.substr(begin + 1, end - begin - 1);
					XTime t(strTmp);
					if (!t.IsErr() && t.tv_sec() >= last)  continue;	// keep live
				}
				strFileName = strDirName + X_PATH_SEPARATOR_CHAR + strName;
				XFileUtil::RemoveFile(strFileName);
			}
			subFinder.FindClose();
			XFileUtil::RemoveEmptyDir(strDirName);
		}
	}
	finder.FindClose();
	return;
}



// ��־ϵͳ��ʼ��
BOOL XLog::Init(const XLogParam& logParam)
{
	return _XLog::Instance()->Init(logParam);
}

//  ������־�������Ϣ����
void XLog::SetPriority(uint32 uPriority)
{
	return _XLog::Instance()->SetPriority(uPriority);
}

uint32 XLog::GetPriority()
{
	return _XLog::Instance()->GetPriority();
}

// ������־���λ��
void XLog::SetOptions(uint32 uOptions/*see XLOG_OPTIONS*/)
{
	return _XLog::Instance()->SetOptions(uOptions);
}

uint32 XLog::GetOptions()
{
	return _XLog::Instance()->GetOptions();
}

// ������־�ı�������
void XLog::SetKeepDays(uint32 uDays)
{
	return _XLog::Instance()->SetKeepDays(uDays);
}

// ��ȡ��־�ı�������
uint32 XLog::GetKeepDays()
{
	return _XLog::Instance()->GetKeepDays();
}

// ����TCP��־�������˿�(0��ʾ����)
BOOL XLog::SetTcpPort(uint32 port)
{
	return _XLog::Instance()->SetTcpPort(port);
}

// ���
BOOL XLog::Printf( uint32 uPriority, const char cszFormat[], ... )
{
	BOOL nRetCode = FALSE;
	va_list marker;
	va_start(marker, cszFormat);
	nRetCode = _XLog::Instance()->Printf(uPriority, cszFormat, marker);
	va_end(marker);
	return nRetCode;
}

BOOL XLog::Printf(uint32 nPriority/*see XLogPriority*/, const char cszFormat[], va_list marker)
{
	BOOL nRetCode = FALSE;
	nRetCode = _XLog::Instance()->Printf(nPriority, cszFormat, marker);
	return nRetCode;
}

} // namespace xbase
