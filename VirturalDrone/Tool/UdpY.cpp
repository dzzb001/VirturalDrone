#include "stdafx.h"
#include "UdpY.h"
#include "TLog.h"

#define Log LogN(1000)

namespace Tool
{

//静态实例对象
CUdpY CUdpY::s_udp;

//构造
CUdpY::CUdpY(void)
{
	m_s = 0;
	m_s = 0;
	m_hThread = NULL;
	m_hStop = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_pfnData = NULL;
	m_lpData = NULL;

#if (defined _WIN32) || (defined _WINDOWS_)
	WSADATA wsaData;
	int nRet = WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
}

//析构
CUdpY::~CUdpY(void)
{
	Stop();
	CloseHandle(m_hStop);
	CloseSocket(m_sManual);

#if (defined _WIN32) || (defined _WINDOWS_)
	int nRet = WSACleanup();
#endif
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	开始，创建自动接收套接字和接收线程
* 输入参数：	nPort			-- 要绑定的端口,如果为0，则会自动分配一个端口
*				strLocalIP		-- 要绑定的本机IP
*				pfnData			-- 接收到数据的回调函数
*				lpData			-- 接收到数据的回调函数环境变量
*				bResusePort		-- RUE：设置端口复用，FALSE：不设置端口复用
* 输出参数：	nPort			-- 实际绑定的端口
* 返 回 值：	执行成功返回true，否则返回false。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2014-12-03	张斌	      	创建
*******************************************************************************/
bool CUdpY::Start(int &nPort, CString strLocalIP, DataRecvCT pfnData, LPVOID lpData, bool bResusePort )
{
	Log("CUdpY::Start(nPort=%d strLocalIP=%s bResusePort=%d)", nPort, strLocalIP, bResusePort);
	if (NULL != m_hThread)
	{
		return false;
	}
	m_pfnData = pfnData;
	m_lpData = lpData;

	//创建socket
	SOCKET_HANDLE s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (bResusePort)
	{
		//设置端口复用
		int bSockReuse = 1;
		if ( 0 != setsockopt(s, 
			SOL_SOCKET,
			SO_REUSEADDR,
			(VAL_TYPE)&bSockReuse, sizeof(bSockReuse)))
		{
			Log("设置端口复用失败");
			CloseSocket(s);
			return false;
		}
	}

	//邦定socket
	sockaddr_in addrRecv;
	addrRecv.sin_family = AF_INET;
	addrRecv.sin_port = htons(nPort);
	addrRecv.sin_addr.s_addr =  IP2Addr((LPCSTR(strLocalIP.GetBuffer(0))));
	if(0 != bind(s, (sockaddr *) &addrRecv, sizeof(addrRecv)))
	{
		Log("端口地址绑定失败<%s:%d>！", strLocalIP, nPort);
		CloseSocket(s);
		return false;
	}

	//设置Socket缓冲(一般默认是8192)
	int nRcvBuffSize = 8192 * 100;
	if(0 != setsockopt(s, SOL_SOCKET, SO_RCVBUF, (VAL_TYPE)&nRcvBuffSize, sizeof(nRcvBuffSize)))
	{
		Log("设置端口缓存大小失败<%s:%d>！", strLocalIP, nPort);
	}

	//如果是自动分配的端口，则获取自动分配的端口号
	if (0 == nPort)
	{
		std::string strIP;
		if (!GetAddr(s, strIP, nPort))
		{
			Log("获取socket<%u>自动分配的端口失败！", s);
			CloseSocket(s);
			return false;
		}
		Log("自动分配套接字端口<%d>", nPort);	
	}

	//保存套接字
	m_s = s;

	//启动接收线程
	ResetEvent(m_hStop);
	m_hThread = AfxBeginThread(TH_Work, this, THREAD_PRIORITY_TIME_CRITICAL)->m_hThread;
	return true;
}

//停止
bool CUdpY::Stop()
{
	Log("CUdpY::Stop()");
	if (NULL != m_hThread)
	{
		SetEvent(m_hStop);
		WaitForSingleObject(m_hThread, INFINITE);
	}
	if (0 != m_s)
	{
		CloseSocket(m_s);
	}
	if (0 != m_sManual)
	{
		CloseSocket(m_sManual);
	}
	return true;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	使用自动接收套接字发送数据
* 输入参数：	pData			-- 待发送的数据指针
*				nLen			-- 待发送数据的长度
* 输出参数：	
* 返 回 值：	发送数据成功返回true，否则返回false。
* 其它说明：	本函数接收数据使用接收的套接字发送数据,所以必须在调用start后、
*				调用stop前调用本接口。
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2014-12-03	张斌	      	创建
*******************************************************************************/
bool CUdpY::Send(const void *pData, int nLen)
{
	//Log("发送：");
	//Log("%s", std::string((char*)pData, nLen > 800 ? 800 : nLen).c_str());
	if ( 0 == m_s)
	{
		return false;
	}
	int nSended = sendto(m_s, 
		(const char*)pData,
		nLen,
		0, 
		(sockaddr*) &m_addrSend, 
		(int)sizeof(m_addrSend));
	return nSended == nLen;
}

/*******************************************************************************
*功    能:	设置自动接收套接字的目标地址
*输入参数:	strIP			-- 数据发送至哪个IP
*			nPort			-- 数据发送至哪个端口
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人		修改内容
--------------------------------------------------------------------------------
2016-7-26	张斌		创建		
*******************************************************************************/
bool CUdpY::SetDstAddr( CString strIP, int nPort )
{
	if (0 == m_s)
	{
		return false;
	}
	m_addrSend.sin_family = AF_INET;
	m_addrSend.sin_port = htons(nPort);
	m_addrSend.sin_addr.s_addr = IP2Addr((LPCSTR)strIP.GetBuffer(0));
	return true;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	设置手动接收套接字的目标发送地址		
* 输入参数：	strIP			-- 数据发送至哪个IP
*				nPort			-- 数据发送至哪个端口
*				strLocalIP		-- 发送数据使用的本机IP
* 输出参数：	
* 返 回 值：	执行成功返回true，否则返回false。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2014-12-03	张斌	      	创建
*******************************************************************************/
bool CUdpY::SetManualDstAddr( CString strIP, int nPort, CString strLocalIP, int nLocalPort /* = 0 */ )
{
	if (0 != m_sManual)
	{
		CloseSocket(m_sManual);
	}
	m_addrSendManual.sin_family = AF_INET;
	m_addrSendManual.sin_port = htons(nPort);
	m_addrSendManual.sin_addr.s_addr = IP2Addr((LPCSTR)strIP.GetBuffer(0));

	//创建socket
	SOCKET_HANDLE s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//邦定socket
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0 == nLocalPort ? 0 : htons(nLocalPort);
	addr.sin_addr.s_addr =  IP2Addr((LPCSTR(strLocalIP.GetBuffer(0))));
	if(0 != bind(s, (sockaddr *) &addr, sizeof(addr)))
	{
		Log("端口地址绑定失败<%s:%d>！", strLocalIP.GetBuffer(0), nPort);
		CloseSocket(s);
		return false;
	}
	//设置Socket缓冲(一般默认是8192)
	int nBuffSize = 8192 * 800;
	if(0 != setsockopt(s, SOL_SOCKET, SO_SNDBUF, (VAL_TYPE)&nBuffSize, sizeof(nBuffSize)))
	{
		Log("设置端口缓存大小失败<%s:%d>！", strLocalIP, nPort);
	}

	m_sManual = s;
	return true;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	使用手动接收套接字发送数据
* 输入参数：	pData			-- 待发送的数据指针
*				nLen			-- 待发送数据的长度
* 输出参数：	
* 返 回 值：	执行成功返回true，否则返回false。
* 其它说明：	本函数应该在调用SetDstAddr后调用
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2014-12-03	张斌	      	创建
*******************************************************************************/
bool CUdpY::SendInManual(const void *pData, int nLen)
{
	if ( 0 == m_sManual)
	{
		return false;
	}
	int nSended = sendto(m_sManual, 
		(const char*)pData,
		nLen,
		0, 
		(sockaddr*) &m_addrSendManual, 
		(int)sizeof(m_addrSendManual));
	return nSended == nLen;
}

/*******************************************************************************
*功    能:	非阻塞接收手动接收套接字的数据
*输入参数:	pBuff			-- 接收数据的缓冲区
*			nSize			-- 接收缓冲区的大小
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-12	张斌			创建		
*******************************************************************************/
int CUdpY::RecvInManual( char *pBuff, int nSize )
{
	timeval timeout = {0, 0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(m_sManual, &fds);
	if(-1 == select((int)m_sManual + 1, &fds, 0, 0, &timeout) )
	{
		Log("select failed!");
		return 0;
	}
	if (!FD_ISSET(m_sManual, &fds))
	{
		return 0;
	}
	sockaddr_in addrSender;
	socklen_t SenderAddrSize = sizeof(addrSender);
	int nCount = recvfrom(m_sManual, 
		pBuff, 
		nSize, 
		0, 
		(sockaddr *)&addrSender, 
		&SenderAddrSize);
	if (0 >= nCount)
	{
		Log("手动接收数据错误，nCount<%d> nErrCode<%d>", nCount, WSAGetLastError());
		return 0;
	}
	return nCount;
}

/*******************************************************************************
*功    能:	获取自动接收套接字的地址信息
*输入参数:	strLocalIP	-- 本地IP
*			nLocalPort	-- 本地端口
*			strDstIP	-- 目的IP
*			nDstPort	-- 目的端口
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人		修改内容
--------------------------------------------------------------------------------
2016-7-28	张斌		创建		
*******************************************************************************/
bool CUdpY::GetAddr( std::string &strLocalIP, int &nLocalPort, std::string &strDstIP, int &nDstPort )
{
	if (0 == m_s)
	{
		return false;
	}
	if (!GetAddr(m_s, strLocalIP, nLocalPort))
	{
		return false;
	}
	nDstPort = htons(m_addrSend.sin_port);
	strDstIP = inet_ntoa(m_addrSend.sin_addr);
	return true;
}

/*******************************************************************************
*功    能:	获取手动接收套接字的地址信息
*输入参数:	strLocalIP	-- 本地IP
*			nLocalPort	-- 本地端口
*			strDstIP	-- 目的IP
*			nDstPort	-- 目的端口
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人		修改内容
--------------------------------------------------------------------------------
2016-7-28	张斌		创建		
*******************************************************************************/
bool CUdpY::GetAddrManual( std::string &strLocalIP, int &nLocalPort, std::string &strDstIP, int &nDstPort )
{
	if (0 == m_sManual)
	{
		return false;
	}
	if (!GetAddr(m_sManual, strLocalIP, nLocalPort))
	{
		return false;
	}
	nDstPort = htons(m_addrSendManual.sin_port);
	strDstIP = inet_ntoa(m_addrSendManual.sin_addr);
	return true;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	接收数据的线程函数
* 输入参数：	lpContext		-- 环境变量
* 输出参数：	
* 返 回 值：	0。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2014-12-03	张斌	      	创建
*******************************************************************************/
UINT CUdpY::TH_Work(LPVOID lpContext)
{
	CUdpY *pThis = (CUdpY*)lpContext;

	//获取网络上的数据
	char RecvBuf[10240] = {0};
	while(WAIT_TIMEOUT == WaitForSingleObject(pThis->m_hStop, 1))
	{
		//检查是否有数据到达
		timeval timeout = {0, 1000};
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(pThis->m_s, &fds);
		if(-1 == select((int)pThis->m_s + 1, &fds, 0, 0, &timeout) )
		{
			Log("select failed!");
			break;
		}
		if (!FD_ISSET(pThis->m_s, &fds))
		{
			continue;
		}

		//有数据到达
		sockaddr_in addrSender;
		socklen_t SenderAddrSize = sizeof(addrSender);
		int nCount = recvfrom(pThis->m_s, 
			RecvBuf, 
			10240, 
			0, 
			(sockaddr *)&addrSender, 
			&SenderAddrSize);
		if (0 >= nCount)
		{
			Log("数据接收错误，nCount<%d> nErrCode<%d>", nCount, WSAGetLastError());
			continue;
		}
		//Log("接收到数据<%d>", nCount);
	
		//回调通知
		if (0 != pThis->m_pfnData)
		{
			pThis->m_pfnData(pThis->m_lpData, RecvBuf, nCount,
				std::string(inet_ntoa(addrSender.sin_addr)), htons(addrSender.sin_port));
		}
	}
	pThis->m_hThread = NULL;
	return 0;
}

//IP地址转换函数
unsigned long CUdpY::IP2Addr(const char * lpszIP)
{
	if (0 == lpszIP || std::string(lpszIP) == "")
	{
		return htonl(INADDR_ANY);
	}
	return inet_addr(lpszIP);
}

//关闭socket
void CUdpY::CloseSocket(SOCKET_HANDLE &s)
{
	if (0 == s)
	{
		return;
	}
#if (defined _WIN32) || (defined _WINDOWS_)
	closesocket(s);
#else
	close(s);
#endif
	s = 0;
}

//获取套接字的绑定地址
bool CUdpY::GetAddr(SOCKET_HANDLE s, std::string &strIP, int &nPort)
{
	sockaddr_in addr;
	int nAddrLen = sizeof(addr);
	int nRet = getsockname(s, (sockaddr*)&addr, &nAddrLen); 
	if (0 != nRet)
	{
		Log("获取套接字<%u>的地址失败 Err<%d>", s, nRet);
		return false;
	}
	nPort = htons(addr.sin_port);
	strIP = inet_ntoa(addr.sin_addr);
	return true;
}

}
