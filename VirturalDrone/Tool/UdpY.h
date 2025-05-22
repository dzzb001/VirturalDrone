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
	
//���յ��������ݵĻص���������
typedef void (*DataRecvCT)(	 LPVOID lpContext,		//��������
							 void *pData,			//���յ�������ָ��
							 int nLen,				//���յ������ݳ���
							 std::string strIP,			//����ԴIP
							 int nPort				//����Դ�˿�
							 );
//UDP���պͷ�����
class CUdpY
{
public:

	//����
	CUdpY(void);

	//����
	~CUdpY(void);

	//��ȡһ��ʵ��
	static CUdpY &GetInstance(){return s_udp;}

	//��ʼ�����������Զ������׽���
	bool Start(
		int &nPort,				//���룺Ҫ�󶨵Ķ˿ڣ���������������0�������ʵ�ʰ󶨵Ķ˿�
		CString strLocalIP,		//Ҫ�󶨵ı���IP
		DataRecvCT pfnData,		//���յ����ݵĻص�����
		LPVOID lpData,			//���յ����ݵĻص�������������
		bool bResusePort		//TRUE�����ö˿ڸ��ã�FALSE�������ö˿ڸ���
		);

	//ֹͣ
	bool Stop();

	//�����Զ������׽��ֵ�Ŀ���ַ
	bool SetDstAddr(
		CString strIP,			//���ݷ������ĸ�IP
		int nPort				//���ݷ������ĸ��˿�
	);

	//ʹ���Զ������׽��ַ�������(��������������ʹ�ý��յ��׽��ַ�������, 
	//���Ա����ڵ���start�󡢵���stopǰ���ñ��ӿ�)
	bool Send(
		const void *pData,		//�����͵�����ָ��
		int nLen				//���������ݵĳ���
		);

	//�����ֶ������׽��ֵ�Ŀ���ַ
	bool SetManualDstAddr(
		CString strIP,			//���ݷ������ĸ�IP
		int nPort,				//���ݷ������ĸ��˿�
		CString strLocalIP,		//��������ʹ�õı���IP
		int nLocalPort = 0			//��������ʹ�õı����˿ڣ�0��ʶ�Զ�����
		);

	//ʹ���ֶ������׽��ַ������ݣ�������Ӧ���ڵ���SetDstAddr����ã�
	bool SendInManual(
		const void *pData,		//�����͵�����ָ��
		int nLen				//���������ݵĳ���
		);

	//�����������ֶ������׽��ֵ�����
	int RecvInManual(
		char *pBuff,			//�������ݵĻ�����
		int nSize				//���ջ������Ĵ�С
		);

	//��ȡ�Զ������׽��ֵĵ�ַ��Ϣ
	bool GetAddr(
		std::string &strLocalIP,	//����IP
		int &nLocalPort,			//���ض˿�
		std::string &strDstIP,		//Ŀ��IP
		int &nDstPort				//Ŀ�Ķ˿�
		);

	//��ȡ�ֶ������׽��ֵĵ�ַ��Ϣ
	bool GetAddrManual(
		std::string &strLocalIP,	//����IP
		int &nLocalPort,			//���ض˿�
		std::string &strDstIP,		//Ŀ��IP
		int &nDstPort				//Ŀ�Ķ˿�
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

	//���ݽ��ջص������ͻ�������
	DataRecvCT m_pfnData;
	LPVOID m_lpData;

	//���ݽ����߳̾����ֹͣ�¼�
	HANDLE m_hThread;
	HANDLE m_hStop;

	//�Զ������׽��ֵĵ�ַ
	sockaddr_in m_addrSend;

	//�Զ������׽���
	SOCKET_HANDLE m_s;
	
	//�ֶ������׽��ֵ�Ŀ�ĵ�ַ
	sockaddr_in m_addrSendManual;

	//�ֶ������׽���
	SOCKET_HANDLE m_sManual;

	//��̬ʵ������
	static CUdpY s_udp;

protected:

	//�������ݵ��̺߳���
	static UINT TH_Work(LPVOID lpContext);

	//IP��ַת������
	unsigned long IP2Addr(const char * lpszIP);

	//�ر�socket
	void CloseSocket(SOCKET_HANDLE &s);

	//��ȡ�׽��ֵİ󶨵�ַ
	bool GetAddr(SOCKET_HANDLE s, std::string &strIP, int &nPort);
};

}
