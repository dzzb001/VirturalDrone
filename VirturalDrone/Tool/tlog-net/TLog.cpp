#include "stdafx.h"

#if defined(_WIN32) || defined(_WINDOWS_)
#pragma warning(disable : 4996)
#pragma warning(disable : 4267)
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdarg.h>
#include "TLog.h"

namespace Tool
{

std::string TLog::m_sstrName = "default";
std::string TLog::m_sstrIP = "127.0.0.1";
int TLog::m_snPort = 621002;
bool TLog::m_sbClosePrint = false;
bool TLog::m_sbCloseNet = true;

TLog::TLog(const char *lpszFile, int nLine, int nLevel)
: m_lpszFile(lpszFile), m_nLine(nLine), m_nLevel(nLevel)
{
}

void TLog::SetConfig(std::string strName, std::string strServerIP, int nPort)
{
	m_sstrName = strName;
	m_sstrIP = strServerIP;
	m_snPort = nPort;
}

bool TLog::SetPrint(bool bClosePrint)
{
	bool bTmp = m_sbClosePrint;
	m_sbClosePrint = bClosePrint;
	return bTmp;
}

bool TLog::SetNetout(bool bCloseNet)
{
	bool bTmp = m_sbCloseNet;
	m_sbCloseNet = bCloseNet;
	return bTmp;
}

void TLog::operator()(const char *pszFmt, ...) const
{
	char pszMsg[1024];
	va_list ptr; 
	va_start(ptr, pszFmt);
	vsprintf(pszMsg, pszFmt, ptr); 
	va_end(ptr);

	if (!m_sbClosePrint)
	{
#if defined(TRACE)
		TRACE("TLog(%d) : %s\n", m_nLevel, pszMsg);
		printf("%s\n", pszMsg);
#else
		printf("%s\n", pszMsg);
#endif
	}

	if (!m_sbCloseNet)
	{
		SendMsg(pszMsg);	
	}
}

#if defined (_WIN32) || defined (_WINDOWS_)
void TLog::operator()(const wchar_t * pszFmt, ...) const
{
	wchar_t pszMsg[1024];
	va_list ptr; 
	va_start(ptr, pszFmt);
	vswprintf(pszMsg, pszFmt, ptr); 
	va_end(ptr);

	char szMsg[1024];
	WideCharToMultiByte(CP_ACP, 0, pszMsg, -1, szMsg, 1024, NULL, NULL);
	(*this)(szMsg);
}
#endif

void TLog::SendMsg(const char * lpszMsg) const
{
	std::string strData;
	NetCmd cmd;
	cmd << m_sstrName;
	cmd << m_nLevel;
	cmd << m_lpszFile;
	cmd << m_nLine;
	cmd << lpszMsg;

#if defined(_WIN32) || defined(_WINDOWS_)
	WSADATA wsaData;
	static bool bInited = false;
	if (!bInited)
	{
		bInited = true;
		WSAStartup(MAKEWORD(2,2), &wsaData);
	}
	SOCKET SendSocket;
#else
	int SendSocket;
#endif
	struct sockaddr_in RecvAddr;
	if (-1 == (SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
	{
		return;
	}
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(m_snPort);
	RecvAddr.sin_addr.s_addr = inet_addr(m_sstrIP.c_str());
	int nSend = sendto(SendSocket, 
		cmd.GetData().data(), 
		cmd.GetData().size(), 
		0, 
		(sockaddr*) &RecvAddr, 
		(int)sizeof(RecvAddr));
#if defined(_WIN32) || defined(_WINDOWS_)
	closesocket(SendSocket);
#else
	close(SendSocket);
#endif
}

}

