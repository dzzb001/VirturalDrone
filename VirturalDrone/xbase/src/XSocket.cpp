// 2008-12-03
// XSocket.cpp
// guoshanhe
// socket简单封装

#include "XSocket.h"

namespace xbase {

#ifdef __WINDOWS__
class XWinSockInit
{
public:
	static XWinSockInit *Instance()
	{
		static XWinSockInit instance;
		return &instance;
	}

private:
	XWinSockInit()
	{
		WORD    wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		int     nRetCode = 0;
		nRetCode = WSAStartup(wVersionRequested, &wsaData);
		ASSERT((!nRetCode) && "WSAStartup failed!");
	}
	~XWinSockInit()
	{
		WSACleanup();
	}	
};

static XWinSockInit *g_pSocketInstance = XWinSockInit::Instance();
#endif//__WINDOWS__

///////////////////////////////////////////////////////////////////////
// // base interfaces
///////////////////////////////////////////////////////////////////////
int XSocket::GetSockType(string &strSockType)	// 0:NULL, 1:TCP, 2:UDP, 3:RAW
{
	int sockType = 0;
	socklen_t optlen = 4;

	if (m_sock == INVALID_SOCKET)
	{
		strSockType = "NULL";
		return 0;
	}

	if (0 != getsockopt(m_sock, SOL_SOCKET, SO_TYPE, (char*)&sockType, &optlen ))
	{
		strSockType = "NULL";
		return 0;
	}

	switch (sockType)
	{
	case 1:
		strSockType = "TCP";
		break;
	case 2:
		strSockType = "UDP";
		break;
	case 3:
		strSockType = "RAW";
		break;
	case 0:
	default:
		strSockType = "NULL";
		break;
	}
	return sockType;
}

BOOL XSocket::Open(int type/* = SOCK_STREAM*/)
{
	Close();

	if ((m_sock = socket(AF_INET, type, 0)) == INVALID_SOCKET)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::Bind(const string& strHost, uint16 uPort)
{
	struct sockaddr_in addr = {};
	string strIP;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	_GetIPByHost(strIP, strHost);
	_ConvertAddr(strIP, uPort, addr);
	return (BOOL)(0 == bind(m_sock, (struct sockaddr *)&addr, sizeof(addr)));
}

BOOL XSocket::Connect(const string& strHost, uint16 uPort)
{
	int ret = -1;
	struct sockaddr_in addr;
	string strIP;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	_GetIPByHost(strIP, strHost);
	_ConvertAddr(strIP, uPort, addr);
	while (1)
	{
		ret = connect(m_sock, (struct sockaddr*)&addr, sizeof(addr));
		if (ret == 0)
		{
			return TRUE;
		}
		else if (_IsSocketCanRestore())
		{
			continue;
		}
		break;
	}
	
#ifdef __WINDOWS__
	if (WSAEINPROGRESS == GetLastError() || 
		WSAEALREADY == GetLastError() || 
		WSAEWOULDBLOCK == WSAGetLastError())
	{
		return TRUE;
	}

#endif//__WINDOWS__
#ifdef __GNUC__
	if (errno == EINPROGRESS || errno == EALREADY)
	{
		return TRUE;
	}
#endif//__GNUC__

	return FALSE;
}

BOOL XSocket::AsyncConnect(const string& strHost, uint16 uPort, int timeoutMilliseconds/* = 10000*/)
{
	BOOL bRet = FALSE;
	int ret = -1;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	bRet = SetNonBlock(1);
	if (!bRet)
	{
		Close();
		return FALSE;
	}
	bRet = Connect(strHost, uPort);
	if (!bRet)
	{
		Close();
		return FALSE;
	}
	ret = CheckCanSend(timeoutMilliseconds);
	if (1 != ret)
	{
		Close();
		return FALSE;
	}
	bRet = SetNonBlock(0);
	if (!bRet)
	{
		Close();
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::Listen(const string& strHost, uint16 uPort, int nQueLen/* = 5*/)
{
	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}
	if(!SetReUseAddr())
	{
		return FALSE;
	}
	if (!Bind(strHost, uPort))
	{
		return FALSE;
	}

	if (0 != listen(m_sock, nQueLen))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::Accept(XSocket &sock)
{
	struct sockaddr_in addr = {};
	socklen_t sockLen = sizeof(addr);
	SOCKET s = INVALID_SOCKET;

	if (m_sock == INVALID_SOCKET)
	{
		sock.Attach(s);
		return FALSE;
	}

	while (TRUE)
	{
		sockLen = sizeof(addr);
		s = accept(m_sock, (sockaddr *)&addr, &sockLen);
		if (s == INVALID_SOCKET)
		{
			if (_IsSocketCanRestore())
			{
				continue;
			}
			else if (_IsSocketWouldBlock())
			{
				sock.Attach(s);
				return TRUE;
			}
			else
			{
				sock.Attach(s);
				return FALSE;
			}
		}
		break;
	}
	
	sock.Attach(s);
	return TRUE;
}

BOOL XSocket::Close(BOOL bLinger/* = FALSE*/, int delaySeconds/* = 0*/)
{
	struct linger lingerStruct = {1, delaySeconds};

	if (m_sock == INVALID_SOCKET)
	{
		return TRUE;
	}

	if (bLinger)
	{
		setsockopt(m_sock, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct));
	}
	
	return _CloseSocket();
}

int XSocket::IsAlive()
{
	int ret = -1;
	volatile int nData = 0; // no use

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	ret = send(m_sock, (char *)&nData, 0, 0);
	if (ret == -1)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::GetLocalAddr(string &strIP, uint16 &uPort)
{
	int ret = -1;
	struct sockaddr_in addr;
	int namelen = sizeof(sockaddr);

	if (m_sock == INVALID_SOCKET)
	{
		strIP = "";
		uPort = 0;
		return FALSE;
	}

	ret = getsockname(m_sock, (struct sockaddr *)&addr, (socklen_t *)&namelen);
	if (ret != 0)
	{
		strIP = "";
		uPort = 0;
		return FALSE;
	}

	strIP = inet_ntoa(addr.sin_addr);
	uPort = ntohs(addr.sin_port);
	return TRUE;
}

string XSocket::GetRemoteIP()
{
	if (m_sock == INVALID_SOCKET)
	{
		return "";
	}
	struct sockaddr_in addr;
	int namelen = sizeof(sockaddr);
	getpeername(m_sock, (struct sockaddr *)&addr, (socklen_t *)&namelen);
	return inet_ntoa(addr.sin_addr);
}

int  XSocket::GetRemotePort()
{
	if (m_sock == INVALID_SOCKET)
	{
		return 0;
	}
	struct sockaddr_in addr;
	int namelen = sizeof(sockaddr);
	getpeername(m_sock, (struct sockaddr *)&addr, (socklen_t *)&namelen);
	return (int)ntohs(addr.sin_port);
}

BOOL XSocket::GetRemoteAddr(string &strIP, uint16 &uPort)
{
	int ret = -1;
	struct sockaddr_in addr;
	int namelen = sizeof(sockaddr);

	if (m_sock == INVALID_SOCKET)
	{
		strIP = "";
		uPort = 0;
		return FALSE;
	}

	ret = getpeername(m_sock, (struct sockaddr *)&addr, (socklen_t *)&namelen);
	if (ret != 0)
	{
		strIP = "";
		uPort = 0;
		return FALSE;
	}

	strIP = inet_ntoa(addr.sin_addr);
	uPort = ntohs(addr.sin_port);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// option interfaces
///////////////////////////////////////////////////////////////////////
BOOL XSocket::SetReUseAddr(int nVal/* = 1*/)
{
	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	if (0 != setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&nVal, sizeof(int)))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::SetKeepAlive(int nVal/* = 1*/)
{
	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	if (0 != setsockopt(m_sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&nVal, sizeof(int)))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::SetNonBlock(int nVal/* = 1*/)
{
	int ret = -1;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

#ifdef __WINDOWS__
	unsigned long ulOption = (unsigned long)nVal;
	ret = ioctlsocket(m_sock, FIONBIO, (unsigned long *)&ulOption);
#endif//__WINDOWS__
#ifdef __GNUC__
	ret = fcntl(m_sock, F_GETFL, 0);
	if (ret == -1) return FALSE;
	if (nVal)
	{
		ret = fcntl(m_sock, F_SETFL, ret | O_NONBLOCK);
	}
	else
	{
		ret = fcntl(m_sock, F_SETFL, ret & (~O_NONBLOCK));
	}
#endif//__GNUC__

	if (ret != 0)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::SetSendBufSize(int nSize/* = 8192*/)
{
	int ret = -1;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	ret = setsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (const char*)&nSize, sizeof(int));
	if (ret != 0)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::GetSendBufSize(int& nSize)
{
	int ret = -1;
	socklen_t optlen = sizeof(int);

	if (m_sock == INVALID_SOCKET)
	{
		nSize = 0;
		return FALSE;
	}

	ret = getsockopt(m_sock, SOL_SOCKET, SO_SNDBUF, (char *)&nSize, &optlen);
	if (ret != 0)
	{
		nSize = 0;
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::SetRecvBufSize(int nSize/* = 8192*/)
{
	int ret = -1;

	if (m_sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	ret = setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&nSize, sizeof(int));
	if (ret != 0)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL XSocket::GetRecvBufSize(int& nSize)
{
	int ret = -1;
	socklen_t optlen = sizeof(int);

	if (m_sock == INVALID_SOCKET)
	{
		nSize = 0;
		return FALSE;
	}

	ret = getsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (char *)&nSize, &optlen);
	if (ret != 0)
	{
		nSize = 0;
		return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////////////////////
// date interfaces
///////////////////////////////////////////////////////////////////////
int XSocket::CheckCanRecv(int timeoutMilliseconds/* = -1*/)
{
	struct timeval timeout = {};
	timeval *pcTimeout = NULL;

	if (m_sock == INVALID_SOCKET)
	{
		return -1;
	}

	if (timeoutMilliseconds >= 0)
	{
		timeout.tv_sec = timeoutMilliseconds / 1000;
		timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;
		pcTimeout = &timeout;
	}

	return _CheckCanRecv(pcTimeout);
}

int XSocket::CheckCanSend(int timeoutMilliseconds/* = -1*/)
{
	struct timeval timeout = {};
	timeval *pcTimeout = NULL;

	if (m_sock == INVALID_SOCKET)
	{
		return -1;
	}

	if (timeoutMilliseconds >= 0)
	{
		timeout.tv_sec = timeoutMilliseconds / 1000;
		timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;
		pcTimeout = &timeout;
	}

	return _CheckCanSend(pcTimeout);
}

int XSocket::Send(const char* buf, int len)
{
	int ret = -1;

	if ((m_sock == INVALID_SOCKET))
	{
		return -1;
	}
	if ((buf == NULL) || (len <= 0))
	{
		return 0;
	}

	while (TRUE)
	{
		ret = send(m_sock, buf, len, 0);
		if (ret < 0)
		{
			if (_IsSocketCanRestore())
			{
				continue;
			}
			if (_IsSocketWouldBlock())
			{
				return 0;
			}
			return -1;
		}
		break;
	}
	
	return ret;
}

int XSocket::Recv(char* buf, int len)
{
	int ret = -1;

	if ((m_sock == INVALID_SOCKET) || (buf == NULL) || (len <= 0))
	{
		return -1;
	}

	while (TRUE)
	{
		ret = recv(m_sock, buf, len, 0);
		if (ret == 0) return -1;
		if (ret < 0)
		{
			if (_IsSocketCanRestore())
			{
				continue;
			}
			if (_IsSocketWouldBlock())
			{
				return 0;
			}
			return -1;
		}
		break;
	}

	return ret;
}

int XSocket::SendTo(const char* buf, int len, const string& strHost, uint16 uPort)
{
	int ret = -1;
	struct sockaddr_in addr;
	string strIP;

	if ((m_sock == INVALID_SOCKET) || (buf == NULL) || (len <= 0))
	{
		return -1;
	}

	_GetIPByHost(strIP, strHost);
	_ConvertAddr(strIP, uPort, addr);

	while (TRUE)
	{
		ret = sendto(m_sock, buf, len, 0, (const struct sockaddr *)&addr, sizeof(addr));
		if (ret < 0)
		{
			if (_IsSocketCanRestore())
			{
				continue;
			}
			if (_IsSocketWouldBlock())
			{
				return 0;
			}
			return -1;
		}
		break;
	}
	return ret;
}

int XSocket::RecvFrom(char* buf, int len, string& strIP, uint16& uPort)
{
	int ret = -1;
	struct sockaddr_in addr;
	socklen_t fromlen = (socklen_t)sizeof(addr);

	if ((m_sock == INVALID_SOCKET) || (buf == NULL) || (len <= 0))
	{
		return -1;
	}

	while (TRUE)
	{
		ret = recvfrom(m_sock, buf, len, 0, (struct sockaddr *)&addr, &fromlen);
		if (ret == 0) return -1;
		if (ret < 0)
		{
			strIP = "";
			uPort = 0;

			if (_IsSocketCanRestore())
			{
				continue;
			}
			if (_IsSocketWouldBlock())
			{
				return 0;
			}
			return ret;
		}
		break;
	}

	strIP = inet_ntoa(addr.sin_addr);
	uPort = ntohs(addr.sin_port);
	return ret;
}

// return -1:error, 0:timeout, >0: send number
int XSocket::SendN(const char* buf, int len, int timeoutMilliseconds/* = -1*/)
{
	int ret = -1;
	int sendsize = 0;

	if (m_sock == INVALID_SOCKET || buf == NULL || len <= 0) return -1;

	while (sendsize < len)
	{
		ret = CheckCanSend(timeoutMilliseconds);
		if (ret == -2) continue;
		if (ret == -1) return -1;
		if (ret == 0) return sendsize;

		ret = Send(buf + sendsize, len - sendsize);
		if (ret < 0)
		{
			return -1;
		}
		sendsize += ret;
	}
	return sendsize;
}

// return -1:error, 0:timeout, >0: recv number
int XSocket::RecvN(char* buf, int len, int timeoutMilliseconds/* = -1*/)
{
	int ret = -1;
	int recvsize = 0;

	if (m_sock == INVALID_SOCKET || buf == NULL) return -1;
	if (len <= 0) return 0;

	while (recvsize < len)
	{
		ret = CheckCanRecv(timeoutMilliseconds);
		if (ret == -2) continue;
		if (ret == -1) return -1;
		if (ret == 0) return recvsize;

		ret = Recv((char *)(buf + recvsize), len - recvsize);
		if (ret < 0)
		{
			return -1;
		}
		recvsize += ret;
	}
	return recvsize;
}

///////////////////////////////////////////////////////////////////////
// private interfaces
///////////////////////////////////////////////////////////////////////
int XSocket::_CheckCanRecv(timeval *pcTimeout)
{
	int ret = -1;
	fd_set FDSet;

	FD_ZERO(&FDSet);
	FD_SET(m_sock, &FDSet);

	// If timeout is NULL (no timeout), select can block indefinitely.
	// In windows, pcTimeout not altered; In linux, pcTimeout may update.
	ret = select((int)m_sock + 1, &FDSet, NULL, NULL, pcTimeout);

	if (ret == 0)
		return 0;

	if (ret > 0)
		return 1;

	if (_IsSocketCanRestore())
	{
		return -2;
	}

	return -1;
}

int XSocket::_CheckCanSend(timeval *pcTimeout)
{
	int ret = -1;
	fd_set FDSet;

	FD_ZERO(&FDSet);
	FD_SET(m_sock, &FDSet);

	// If timeout is NULL (no timeout), select can block indefinitely.
	// In windows, pcTimeout not altered; In linux, pcTimeout may update.
	ret = select((int)m_sock + 1, NULL, &FDSet, NULL, pcTimeout);

	if (ret == 0)
		return 0;

	if (ret > 0)
		return 1;

	if (_IsSocketCanRestore())
	{
		return -2;
	}

	return -1;
}

BOOL XSocket::_GetIPByHost(string &strIP, const string &strHost)
{
	int ret = -1;
	struct in_addr ip;
	uint32 a, b, c, d;
	struct hostent *pHost = NULL;
	char buf[64];

	strIP.clear();

	if (strHost.empty())
	{
		// strip is "0.0.0.0"
		return TRUE;
	}

	ret = sscanf(strHost.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
	if (ret != 4)
	{
		pHost = gethostbyname(strHost.c_str());
		if (pHost && pHost->h_addr)
		{
			ip = *(in_addr *)pHost->h_addr;
			strIP = inet_ntoa(ip);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		sprintf(buf, "%u.%u.%u.%u", a, b, c, d);
		strIP = buf;
	}
	return TRUE;
}

void XSocket::_ConvertAddr(const string &strIP, uint16 uPort, sockaddr_in &addr)
{
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(uPort);
	addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	if (addr.sin_addr.s_addr == INADDR_NONE)
	{
		addr.sin_addr.s_addr = INADDR_ANY;
	}
}

#ifdef __WINDOWS__
BOOL XSocket::_IsSocketCanRestore()
{
	return (BOOL)(WSAGetLastError() == EINTR);
}

BOOL XSocket::_IsSocketWouldBlock()
{
	return (BOOL)(
		(WSAEWOULDBLOCK == WSAGetLastError()) ||
		(WSA_IO_PENDING == WSAGetLastError())
		);
}

BOOL XSocket::_CloseSocket()
{
	BOOL bRet = TRUE;
	if (m_sock != INVALID_SOCKET)
	{
		bRet = (BOOL)(!closesocket(m_sock));
	}
	m_sock = INVALID_SOCKET;
	return bRet;
}
#endif//__WINDOWS__

#ifdef __GNUC__
BOOL XSocket::_IsSocketCanRestore()
{
	return (BOOL)(errno == EINTR);
}

BOOL XSocket::_IsSocketWouldBlock()
{
	return (BOOL)(EAGAIN == errno || EWOULDBLOCK == errno);
}

BOOL XSocket::_CloseSocket()
{
	BOOL bRet = TRUE;
	if (m_sock != INVALID_SOCKET)
	{
		bRet = (BOOL)(!close(m_sock));
	}
	m_sock = INVALID_SOCKET;
	return bRet;
}

#endif//__GNUC__

// 获到本机名和IP地址
BOOL X_GetLocalHost(string& strHostName, vector<string>& vHostAddrs)
{
	strHostName.clear();
	vHostAddrs.clear();

	char name[1024] = {};
	if (SOCKET_ERROR == gethostname(name, 1023)) return FALSE;
	strHostName = name;

	struct hostent* ph = NULL;
	ph = gethostbyname(name);
	if (ph == NULL) return FALSE;
	if (ph->h_length != 4) return FALSE;

	int i = 0;
	while (ph->h_addr_list[i])
	{
		BYTE *pAddr = (BYTE *)ph->h_addr_list[i];
		sprintf(name, "%u.%u.%u.%u", *(pAddr + 0), *(pAddr + 1), *(pAddr + 2), *(pAddr + 3));
		vHostAddrs.push_back(name);
		i++;
	}
	return TRUE;
}
//
void XSocket::SetSendTimeout( int nSecond )
{
	int timeout = nSecond * 1000;
	::setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
}

void XSocket::SetRecvTimeout( int nSecond )
{
	int timeout = nSecond * 1000;
	::setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}


} // namespace xbase
