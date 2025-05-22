#pragma once

#if (defined _WIN32) || (defined _WINDOWS_)
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <pthread.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef void            *LPVOID;
#endif

namespace Tool
{
	
//接收到网络数据的回调函数定义
typedef void (*DataRecvCT)(	 LPVOID lpContext,		//环境变量
							 void *pData,			//接收到的数据指针
							 int nLen,				//接收到的数据长度
							 std::string strIP,			//数据源IP
							 int nPort				//数据源端口
							 );
//UDP接收和发送类
class CUdpY
{
public:

	//构造
	CUdpY(void);

	//析构
	~CUdpY(void);

	//获取一个实例
	static CUdpY &GetInstance(){return s_udp;}

	//开始，创建接收自动接收套接字
	bool Start(
		int &nPort,				//输入：要绑定的端口，输出：如果输入是0，则输出实际绑定的端口
		CString strLocalIP,		//要绑定的本机IP
		DataRecvCT pfnData,		//接收到数据的回调函数
		LPVOID lpData,			//接收到数据的回调函数环境变量
		bool bResusePort		//TRUE：设置端口复用，FALSE：不设置端口复用
		);

	//停止
	bool Stop();

	//设置自动接收套接字的目标地址
	bool SetDstAddr(
		CString strIP,			//数据发送至哪个IP
		int nPort				//数据发送至哪个端口
	);

	//使用自动接收套接字发送数据(本函数接收数据使用接收的套接字发送数据, 
	//所以必须在调用start后、调用stop前调用本接口)
	bool Send(
		const void *pData,		//待发送的数据指针
		int nLen				//待发送数据的长度
		);

	//设置手动接收套接字的目标地址
	bool SetManualDstAddr(
		CString strIP,			//数据发送至哪个IP
		int nPort,				//数据发送至哪个端口
		CString strLocalIP,		//发送数据使用的本机IP
		int nLocalPort = 0			//发送数据使用的本机端口，0标识自动分配
		);

	//使用手动接收套接字发送数据（本函数应该在调用SetDstAddr后调用）
	bool SendInManual(
		const void *pData,		//待发送的数据指针
		int nLen				//待发送数据的长度
		);

	//非阻塞接收手动接收套接字的数据
	int RecvInManual(
		char *pBuff,			//接收数据的缓冲区
		int nSize				//接收缓冲区的大小
		);

	//获取自动接收套接字的地址信息
	bool GetAddr(
		std::string &strLocalIP,	//本地IP
		int &nLocalPort,			//本地端口
		std::string &strDstIP,		//目的IP
		int &nDstPort				//目的端口
		);

	//获取手动接收套接字的地址信息
	bool GetAddrManual(
		std::string &strLocalIP,	//本地IP
		int &nLocalPort,			//本地端口
		std::string &strDstIP,		//目的IP
		int &nDstPort				//目的端口
		);

protected:

#if (defined _WIN32) || (defined _WINDOWS_)
	typedef SOCKET SOCKET_HANDLE;
	typedef const char * VAL_TYPE;
	typedef HANDLE	THREAD_HANDLE;
	typedef UINT THREAD_RET;
#define INVALID_TH_HANDLE (THREAD_HANDLE)(0)
#else
	typedef int SOCKET_HANDLE;
	typedef const void * VAL_TYPE;
	typedef pthread_t	THREAD_HANDLE;
	typedef void * THREAD_RET;
#define INVALID_TH_HANDLE (THREAD_HANDLE)(-1)
#endif

	//数据接收回调函数和环境变量
	DataRecvCT m_pfnData;
	LPVOID m_lpData;

	//数据接收线程句柄和停止事件
	HANDLE m_hThread;
	HANDLE m_hStop;

	//自动接收套接字的地址
	sockaddr_in m_addrSend;

	//自动接收套接字
	SOCKET_HANDLE m_s;
	
	//手动接收套接字的目的地址
	sockaddr_in m_addrSendManual;

	//手动接收套接字
	SOCKET_HANDLE m_sManual;

	//静态实例对象
	static CUdpY s_udp;

protected:

	//接收数据的线程函数
	static UINT TH_Work(LPVOID lpContext);

	//IP地址转换函数
	unsigned long IP2Addr(const char * lpszIP);

	//关闭socket
	void CloseSocket(SOCKET_HANDLE &s);

	//获取套接字的绑定地址
	bool GetAddr(SOCKET_HANDLE s, std::string &strIP, int &nPort);
};

}
