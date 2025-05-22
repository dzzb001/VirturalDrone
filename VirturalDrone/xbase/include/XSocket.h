// 2008-12-03
// XSocket.h
// guoshanhe
// socket简单封装

#pragma once

#ifndef _X_SOCKET_H_
#define _X_SOCKET_H_

#include "XDefine.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XSocket
////////////////////////////////////////////////////////////////////////////////
class XSocket
{
public:
	/// constructor and destructor
	XSocket() : m_sock(INVALID_SOCKET) {}
	XSocket(SOCKET sock) : m_sock(sock) {}
	~XSocket() { Close(); }

public:
	/// attribute interfaces
	XSocket& operator=(SOCKET sock)	{ if (m_sock != sock) { Close(); m_sock = sock; } return *this; }
	operator SOCKET() { return m_sock; }
	SOCKET GetHandle() { return m_sock; }
	void Attach(SOCKET sock) { if (m_sock != sock) { Close(); m_sock = sock; } }
	SOCKET Detach()	{ SOCKET sock = m_sock; m_sock = INVALID_SOCKET; return sock; }
	int GetSockType(string &strSockType);	// 0:NULL, 1:TCP, 2:UDP, 3:RAW

public:
	/// base interfaces
	BOOL Open(int type = SOCK_STREAM);
	BOOL Bind(const string& strHost, uint16 uPort);
	BOOL Connect(const string& strHost, uint16 uPort);
	BOOL AsyncConnect(const string& strHost, uint16 uPort, int timeoutMilliseconds = 10000);
	BOOL Listen(const string& strHost, uint16 uPort, int nQueLen = 5);
	BOOL Accept(XSocket &sock);
	BOOL Close(BOOL bLinger = FALSE, int delaySeconds = 0);
	BOOL IsAlive();		// Only use at TCP
	BOOL GetLocalAddr(string &strIP, uint16 &uPort);
	BOOL GetRemoteAddr(string &strIP, uint16 &uPort);
	string GetRemoteIP();
	int  GetRemotePort();
	BOOL IsWouldBlock() { return _IsSocketWouldBlock(); }

public:
	/// option interfaces
	// nVal = 1 is set, nVal = 0 is reset.
	BOOL SetReUseAddr(int nVal = 1);
	BOOL SetKeepAlive(int nVal = 1);
	BOOL SetNonBlock(int nVal = 1);
	
	BOOL SetSendBufSize(int nSize = 8192);
	BOOL GetSendBufSize(int& nSize);
	BOOL SetRecvBufSize(int nSize = 8192);
	BOOL GetRecvBufSize(int& nSize);

public:
	/// date interfaces
	// return -2: again, -1: error, 0: timeout, 1: success
	int CheckCanRecv(int timeoutMilliseconds = -1);
	int CheckCanSend(int timeoutMilliseconds = -1);
	
	// return >=0: send number of bytes, -1: error or peer host closed
	int Send(const char* buf, int len);
	// return >=0: recv number of bytes, -1: error or peer host closed
	int Recv(char* buf, int len);
	// return >=0: sendto number of bytes, -1: error or peer host closed
	int SendTo(const char* buf, int len, const string& strHost, uint16 uPort);
	// return >=0: recvfrom number of bytes, -1: error or peer host closed
	int RecvFrom(char* buf, int len, string& strIP, uint16& uPort);

	// return -1:error, 0:timeout, >0: send number
	int SendN(const char* buf, int len, int timeoutMilliseconds = -1);
	// return -1:error, 0:timeout, >0: recv number
	int RecvN(char* buf, int len, int timeoutMilliseconds = -1);
	//超时设置
	void  SetSendTimeout( int nSecond );
	void  SetRecvTimeout( int nSecond );
	

private:
	// return -2: again, -1: error, 0: timeout, 1: success
	int _CheckCanRecv(timeval *pcTimeout = NULL);
	int _CheckCanSend(timeval *pcTimeout = NULL);

	BOOL _GetIPByHost(string &strIP, const string &strHost);
	void _ConvertAddr(const string &strIP, uint16 uPort, sockaddr_in &addr);
	BOOL _IsSocketCanRestore();
	BOOL _IsSocketWouldBlock();
	BOOL _CloseSocket();

private:
	XSocket(const XSocket&);
	XSocket& operator=(const XSocket&);

private:
	SOCKET			m_sock;
};

// 获到本机名和IP地址
BOOL X_GetLocalHost(string& strHostName, vector<string>& vHostAddrs);

} // namespace xbase

using namespace xbase;

#endif//_X_SOCKET_H_
