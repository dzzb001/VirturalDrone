/*******************************************************************************

    File : BaseSocket.cpp
    
*******************************************************************************/
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <poll.h>
#include "BaseSocket.h"
#include "MemRecord.h"

namespace common {
////////////////////////////////////////////////////////////////////////////////
// Socket
////////////////////////////////////////////////////////////////////////////////
Socket::Socket()
{
    m_nSockID = -1;
    m_bBlock = true;
    m_usLocalPort = 0;
    m_usRemotePort = 0;
}

Socket::~Socket()
{
    Close();
}

int Socket::OpenSocket(int nType)
{
    if((m_nSockID = ::socket(AF_INET, nType, 0)) != -1)
        return RET_OK;
    else
        return RET_ERR;
}

bool Socket::IsValid()
{
	return m_nSockID > 0;
}

void Socket::ReUseAddr()
{
    int nVal = 1;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_REUSEADDR, (char*)&nVal, sizeof(int));
}

int Socket::Bind(const char* szIP, USHORT usPort)
{
    int nRet = 0;
    struct sockaddr_in addr;
    socklen_t addrLen = 0;
    
    SocketUtils::ConvertAddr(szIP, usPort, &addr);
    nRet = ::bind(m_nSockID, (sockaddr*)&addr, sizeof(addr));
    if(nRet == -1)
        return RET_ERR;
    
    if(szIP != NULL)
    {
        m_sLocalIP = szIP;
    }
    if(usPort != 0)
    {
        m_usLocalPort = usPort;
    }
    else
    {
        addrLen = sizeof(addr);
        if(::getsockname(m_nSockID, (sockaddr*)&addr, &addrLen) != -1)
        {
            m_usLocalPort = ntohs(addr.sin_port);
        }
    }
    
    return RET_OK;
}

void Socket::NoDelay()
{
    int nVal = 1;
    ::setsockopt(m_nSockID, IPPROTO_TCP, TCP_NODELAY, (char*)&nVal, sizeof(int));    
}

void Socket::KeepAlive()
{
    int nVal = 1;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_KEEPALIVE, (char*)&nVal, sizeof(int));
}

void Socket::SetSendTimeout(int nSecond)
{
    timeval tv;
    tv.tv_sec = nSecond; 
    tv.tv_usec = 0;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
}

void Socket::SetRecvTimeout(int nSecond)
{   
    timeval tv;
    tv.tv_sec = nSecond;
    tv.tv_usec = 0;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));
}

void Socket::SetSendBufSize(int nSize)
{
    int nVal = nSize;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_SNDBUF, (char*)&nVal, sizeof(int));
}

void Socket::SetRecvBufSize(int nSize)
{
    int nVal = nSize;
    ::setsockopt(m_nSockID, SOL_SOCKET, SO_RCVBUF, (char*)&nVal, sizeof(int));
}

int Socket::Connect(const char* szIP, USHORT usPort)
{
    int nRet = 0;
    struct sockaddr_in addrRemote;

    SocketUtils::ConvertAddr(szIP, usPort, &addrRemote);    
    nRet = ::connect(m_nSockID, (sockaddr*)&addrRemote, sizeof(addrRemote));
    if(nRet == 0 || errno == EINPROGRESS)
    {
        m_usRemotePort = usPort;
        m_sRemoteIP = szIP;
        return RET_OK;
    }
    else
    {
        return RET_ERR;
    }
}

int Socket::Send(const void* pBuf, int nBufSize, int& nSndSize)
{
    if((nSndSize = ::send(m_nSockID, pBuf, nBufSize, 0)) != -1)
    {
        return RET_OK;
    }
    else
    {
        nSndSize = 0;
        // EAGAIN means the requested operation would block
        // EINTR means A signal occurred before any data was transmitted
        if(errno == EAGAIN || errno == EINTR)
        {
            return RET_OK;
        }
        else
        {
            return RET_ERR;
        }
    }
    
}

int Socket::SendN(const void* pBuf, int n)
{
    if (n < 0 || NULL == pBuf)
        return -1;
        
    if (0 == n)
        return 0;
    
    char* p = (char*)pBuf;
    
    int w = 0;
    
    do
    {
        int s = 0;
        
        if (RET_OK != Send(p, n, s))
            break;
        
        n -= s;
        p += s;
        w += s;
    } while (n > 0);
    
    return w;
}

int Socket::WriteV(const struct iovec iov[], int nCnt, int& nWrtSize)
{
    if((nWrtSize = ::writev(m_nSockID, iov, nCnt)) != -1)
    {
        return RET_OK;

    }
    else
    {
        nWrtSize = 0;
        // EAGAIN means the requested operation would block
        // EINTR means A signal occurred before any data was transmitted
        if(errno == EAGAIN || errno == EINTR)
        {
            return RET_OK;
        }
        else
        {
            return RET_ERR;
        }
    }
}

int Socket::Recv(char* szBuf, int nBufSize, int& nRcvSize, int& nSysErrno, int nFlags)
{
	nSysErrno = 0;
    if((nRcvSize = ::recv
		(m_nSockID, szBuf, nBufSize, nFlags)) > 0)
    {
        return RET_OK;
    }

	nSysErrno = errno;	
    if(nRcvSize == -1)
    {
        nRcvSize = 0;        
        // EAGAIN means the receive operation would block
        // EINTR means the receive was interrupted by a signal
        if(nSysErrno == EAGAIN || nSysErrno == EINTR)
        {
            return RET_OK;
        }
        else
        {
            return RET_ERR;
        }
    }
     /* nRcvSize == 0 */
    // zero means the peer has performed an orderly shutdown
    return RET_ERR;
}

int Socket::RecvN(void* buf, int n)
{
    if (NULL == buf || n < 0)
        return -1;
    
    if (0 == n)
        return 0;
    
    char* p = (char*)buf;
    int r = 0;
    
	int nSysErrno = 0;
    do
    {
        int t = 0;
        
        if (RET_OK != Recv((char*)p, n, t, nSysErrno))
            break;
        
        n -= t;
        r += t;
        p += t;
    } while (n > 0);
    
    return r;
}

void Socket::Close()
{
    if(m_nSockID != -1)
    {
        ::shutdown(m_nSockID, 2);
        ::close(m_nSockID);
        m_nSockID = -1;
    }
    m_bBlock = true;
    m_sLocalIP.clear();
    m_sRemoteIP.clear();
    m_usLocalPort = 0;
    m_usRemotePort = 0;
}

void Socket::SetBlock(bool bBlock)
{
    int nFlag = 0;
    
    if(bBlock)
    {
        if(!m_bBlock)
        {
            nFlag = ::fcntl(m_nSockID, F_GETFL, 0);
            nFlag &= ~O_NONBLOCK;
            ::fcntl(m_nSockID, F_SETFL, nFlag);
            m_bBlock = !m_bBlock;
        }
    }
    else
    {
        if(m_bBlock)
        {
            nFlag = ::fcntl(m_nSockID, F_GETFL, 0);
            ::fcntl(m_nSockID, F_SETFL, nFlag | O_NONBLOCK);
            m_bBlock = !m_bBlock;
        }
    }
}

bool Socket::GetBlock() const
{
    return m_bBlock;
}

void Socket::SetSocket(SOCKET nSocket, const sockaddr_in* addrRemote)
{
    in_addr addr;
    
    m_nSockID = nSocket;
    m_usRemotePort = ntohs(addrRemote->sin_port);
    addr.s_addr = addrRemote->sin_addr.s_addr;
    m_sRemoteIP = inet_ntoa(addr);
}

void Socket::SetSocket(Socket* pSocket)
{
    m_nSockID = pSocket->m_nSockID;
    m_bBlock = pSocket->m_bBlock;
    m_sLocalIP = pSocket->m_sLocalIP;
    m_usLocalPort = pSocket->m_usLocalPort;
    m_sRemoteIP = pSocket->m_sRemoteIP;
    m_usRemotePort = pSocket->m_usRemotePort;

    pSocket->m_nSockID = -1;
    pSocket->Close();
}

static int WaitEvent(int sock, int event, int ms)
{
    int ret = -1;
    struct pollfd poller;
    poller.fd = sock;
    poller.events = event;
    poller.revents = 0;
    
    for (;;)
    {
        ret = ::poll(&poller, 1, ms);
        if (-1 == ret)
        {
            if(errno == EINTR)
                continue;
            return RET_ERR;
        }
        
        if (0 == ret)
        {
            return RET_TIMEOUT;
        }
        return RET_OK;
    }
}

int Socket::WaitForSend(int ms)
{
    return WaitEvent(m_nSockID, POLLOUT, ms);
}

int Socket::WaitForRecv(int ms)
{
    return WaitEvent(m_nSockID, POLLIN, ms);
}

bool IsLanIP(const string& ip)
{
	if(ip.empty())
		return false;
	unsigned int a, b, c, d;
	if (4 != sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d))
	{
		return false;
	}
	if (a > 255 || b > 255 || c > 255 || d > 255)
	{
		return false;
	}

	if(a == 192 && b == 168)
		return true;
	else if(a == 172 && (b >= 16  && b <= 31))
		return true;
	else if(a == 10)
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////
// CTCPSocket
////////////////////////////////////////////////////////////////////////////////
TCPSocket::TCPSocket()
{
}

TCPSocket::~TCPSocket()
{
}

int TCPSocket::Listen(const char* szIP, USHORT usPort, int nQueLen)
{
    ReUseAddr();
    if(Bind(szIP, usPort) != RET_OK)
    {
        Close();
        return RET_ERR;
    }
    ::listen(m_nSockID, nQueLen);
    
    return RET_OK;
}

int TCPSocket::Accept(TCPSocket* pSocket)
{
    struct sockaddr_in addr;
    socklen_t sockLen = sizeof(addr);
    SOCKET nNewSockID = -1;
    
    if(pSocket == NULL)
        return RET_ERR;
    
    nNewSockID = ::accept(m_nSockID, (sockaddr*)&addr, &sockLen);
    if(nNewSockID == -1)
        return RET_ERR;

    pSocket->SetSocket(nNewSockID, &addr);    
    return RET_OK;
}


////////////////////////////////////////////////////////////////////////////////
// UDPSocket
////////////////////////////////////////////////////////////////////////////////
UDPSocket::UDPSocket()
{
}

UDPSocket::~UDPSocket()
{
}

int UDPSocket::SendTo(const char* szIP, USHORT usPort, const void* pBuf, 
    int nBufSize, int& nSndSize)
{
    struct sockaddr_in addrRemote;

    SocketUtils::ConvertAddr(szIP, usPort, &addrRemote);
    nSndSize = ::sendto(m_nSockID, pBuf, nBufSize, 0, (sockaddr*)&addrRemote, 
        sizeof(addrRemote));
    if(nSndSize > 0)
    {
        return RET_OK;
    }
    else
    {
        nSndSize = 0;
        // EAGAIN means the requested operation would block
        // EINTR means A signal occurred before any data was transmitted
        if(errno == EAGAIN || errno == EINTR)
        {
            return RET_OK;
        }
        else
        {
            return RET_ERR;
        }        
    }
}

int UDPSocket::RecvFrom(string& sIP, USHORT& usPort, void* pBuf, int nBufSize, 
    int& nRcvSize)
{
    struct sockaddr_in addrRemote;
    socklen_t sockLen = sizeof(addrRemote);

    memset(&addrRemote, 0, sockLen);
    nRcvSize = ::recvfrom(m_nSockID, pBuf, nBufSize, 0, (sockaddr*)&addrRemote, 
            &sockLen);
    if(nRcvSize > 0)
    {
        SocketUtils::AddrToStr(addrRemote, sIP);
        usPort = ntohs(addrRemote.sin_port);
        return RET_OK;
    }
    else if(nRcvSize == -1)
    {
        nRcvSize = 0;        
        // EAGAIN means the receive operation would block
        // EINTR means the receive was interrupted by a signal
        if(errno == EAGAIN || errno == EINTR)
        {
            return RET_OK;
        }
        else
        {
            return RET_ERR;
        }
    }
    else /* nRcvSize == 0 */
    {
        // zero means the peer has performed an orderly shutdown
        return RET_ERR;
    }

}


////////////////////////////////////////////////////////////////////////////////
// SocketUtils
////////////////////////////////////////////////////////////////////////////////
Mutex SocketUtils::m_mutexBind;
void SocketUtils::ConvertAddr(const char* szIP, USHORT usPort, sockaddr_in* pstAddr)
{
    memset(pstAddr, 0, sizeof(sockaddr_in));
    pstAddr->sin_family = AF_INET;
    pstAddr->sin_port = htons(usPort);
    if(szIP != NULL)
        pstAddr->sin_addr.s_addr = inet_addr(szIP);
    else
        pstAddr->sin_addr.s_addr = 0;
}

void SocketUtils::AddrToStr(const sockaddr_in& addr, string& sIP)
{
    sIP = inet_ntoa(addr.sin_addr);
}

int SocketUtils::BindUDPSocketPair(UDPSocket* pRTPSocket, UDPSocket* pRTCPSocket)
{
    static USHORT usCurPort = LOWEST_UDP_PORT;
    USHORT usStartPort = usCurPort;
    MutexLocker locker(&m_mutexBind);

    pRTPSocket->Open();
    pRTCPSocket->Open();
    pRTPSocket->SetBlock(false);
    pRTCPSocket->SetBlock(false);
    
    for(; usStartPort < HIGHEST_UDP_PORT; usStartPort += 2)
    {
        if(pRTPSocket->Bind(NULL, usStartPort) == RET_OK)
        {
            if(pRTCPSocket->Bind(NULL, usStartPort + 1) == RET_OK)
            {
                usCurPort = usStartPort + 2;
                return RET_OK;
            }
        }
    }

    
    for(usStartPort = LOWEST_UDP_PORT; usStartPort < usCurPort; usStartPort += 2)
    {
        if(pRTPSocket->Bind(NULL, usStartPort) == RET_OK)
        {
            if(pRTCPSocket->Bind(NULL, usStartPort + 1) == RET_OK)
            {
                usCurPort = usStartPort + 2;
                return RET_OK;
            }
        }

    }

    pRTPSocket->Close();
    pRTCPSocket->Close();

    return RET_ERR;
}


} // namespace common

