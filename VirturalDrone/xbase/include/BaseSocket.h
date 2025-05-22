/*******************************************************************************

    File : BaseSocket.h
    
*******************************************************************************/
#ifndef MEGA_BASE_SOCKET_H
#define MEGA_BASE_SOCKET_H

#include <list>
#include <string>
#include <sys/uio.h>
#include <netinet/in.h>
#include "BaseDefine.h"
#include "Mutex.h"
using namespace std;

namespace common {
////////////////////////////////////////////////////////////////////////////////
// Socket
////////////////////////////////////////////////////////////////////////////////
class Socket
{
public :
    Socket();
    virtual ~Socket();
    virtual int  Open() = 0;
	bool IsValid();
    void ReUseAddr();
    int  Bind(const char* szIP, USHORT usPort);
    void NoDelay();
    void KeepAlive();
    void SetSendTimeout(int nSecond);
    void SetRecvTimeout(int nSecond);
    void SetSendBufSize(int nSize);
    void SetRecvBufSize(int nSize);
    int  Connect(const char* szIP, USHORT usPort);  
	//int  Connect(const char* szIP, USHORT usPort, long lTimeout );  
    int  Send(const void* pBuf, int nBufSize, int& nSndSize);
    int  SendN(const void* pBuf, int n);
    
    int  WriteV(const struct iovec[], int nCnt, int& nWrtSize);
    int  Recv(char* szBuf, int nBufSize, int& nRcvSize, int& nSysErrno, int nFlags = 0);
    int  RecvN(void* buf, int n);
    
    void Close();
    void SetBlock(bool bBlock);
    bool GetBlock() const;    
    int  GetHandle()                        { return m_nSockID; }   
    const char* GetLocalIP()                { return m_sLocalIP.c_str(); }
    const char* GetRemoteIP()               { return m_sRemoteIP.c_str(); }
    USHORT GetLocalPort()                   { return m_usLocalPort; }
    USHORT GetRemotePort()                  { return m_usRemotePort; }
    void SetSocket(SOCKET nSocket, const sockaddr_in* addrRemote);
    void SetSocket(Socket* pSocket);
    
    int WaitForSend(int ms);
    int WaitForRecv(int ms);
protected :
    int  OpenSocket(int nType);
protected :
    SOCKET      m_nSockID;
    bool        m_bBlock;
    string      m_sLocalIP;
    USHORT      m_usLocalPort;
    string      m_sRemoteIP;
    USHORT      m_usRemotePort;
};

bool IsLanIP(const string& ip);

////////////////////////////////////////////////////////////////////////////////
// TCPSocket
////////////////////////////////////////////////////////////////////////////////
class TCPSocket : public Socket
{
public : 
    TCPSocket();
    virtual ~TCPSocket();
    virtual int Open()      { return OpenSocket(SOCK_STREAM); }
    int  Listen(const char* szIP, USHORT usPort, int nQueLen);
    int  Accept(TCPSocket* pSocket);
};


////////////////////////////////////////////////////////////////////////////////
// UDPSocket
////////////////////////////////////////////////////////////////////////////////
class UDPSocket : public Socket
{
public :
    UDPSocket();
    virtual ~UDPSocket();
    virtual int Open()      { return OpenSocket(SOCK_DGRAM); }
    int SendTo(const char* szIP, USHORT usPort, const void* pBuf, int nBufSize, int& nSndSize);
    int RecvFrom(string& sIP, USHORT& usPort, void* pBuf, int nBufSize, int& nRcvSize);
};


////////////////////////////////////////////////////////////////////////////////
// SocketUtils
////////////////////////////////////////////////////////////////////////////////
class SocketUtils
{
public :
    enum { 
        EV_NULL     = 0,
        EV_READ     = 1, 
        EV_WRITE    = 2,
        EV_ERROR    = 4, 
        EV_TIMEOUT  = 8
    };
    
public :
    static void ConvertAddr(const char* szIP, USHORT usPort, sockaddr_in* pstAddr);
    static void AddrToStr(const sockaddr_in& addr, string& sIP);
    static int  BindUDPSocketPair(UDPSocket* pRTPSocket, UDPSocket* pRTCPSocket);

protected :
    enum 
    {
        LOWEST_UDP_PORT       = 7000,  
        HIGHEST_UDP_PORT      = 65535
    };
    static Mutex        m_mutexBind;
};

}// namespace common

#endif
