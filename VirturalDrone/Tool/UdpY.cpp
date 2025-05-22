#include "stdafx.h"
#include "UdpY.h"
#include "TLog.h"

#define Log LogN(1000)

namespace Tool
{

//��̬ʵ������
CUdpY CUdpY::s_udp;

//����
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

//����
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
* �������ƣ�	
* ����������	��ʼ�������Զ������׽��ֺͽ����߳�
* ���������	nPort			-- Ҫ�󶨵Ķ˿�,���Ϊ0������Զ�����һ���˿�
*				strLocalIP		-- Ҫ�󶨵ı���IP
*				pfnData			-- ���յ����ݵĻص�����
*				lpData			-- ���յ����ݵĻص�������������
*				bResusePort		-- RUE�����ö˿ڸ��ã�FALSE�������ö˿ڸ���
* ���������	nPort			-- ʵ�ʰ󶨵Ķ˿�
* �� �� ֵ��	ִ�гɹ�����true�����򷵻�false��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2014-12-03	�ű�	      	����
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

	//����socket
	SOCKET_HANDLE s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (bResusePort)
	{
		//���ö˿ڸ���
		int bSockReuse = 1;
		if ( 0 != setsockopt(s, 
			SOL_SOCKET,
			SO_REUSEADDR,
			(VAL_TYPE)&bSockReuse, sizeof(bSockReuse)))
		{
			Log("���ö˿ڸ���ʧ��");
			CloseSocket(s);
			return false;
		}
	}

	//�socket
	sockaddr_in addrRecv;
	addrRecv.sin_family = AF_INET;
	addrRecv.sin_port = htons(nPort);
	addrRecv.sin_addr.s_addr =  IP2Addr((LPCSTR(strLocalIP.GetBuffer(0))));
	if(0 != bind(s, (sockaddr *) &addrRecv, sizeof(addrRecv)))
	{
		Log("�˿ڵ�ַ��ʧ��<%s:%d>��", strLocalIP, nPort);
		CloseSocket(s);
		return false;
	}

	//����Socket����(һ��Ĭ����8192)
	int nRcvBuffSize = 8192 * 100;
	if(0 != setsockopt(s, SOL_SOCKET, SO_RCVBUF, (VAL_TYPE)&nRcvBuffSize, sizeof(nRcvBuffSize)))
	{
		Log("���ö˿ڻ����Сʧ��<%s:%d>��", strLocalIP, nPort);
	}

	//������Զ�����Ķ˿ڣ����ȡ�Զ�����Ķ˿ں�
	if (0 == nPort)
	{
		std::string strIP;
		if (!GetAddr(s, strIP, nPort))
		{
			Log("��ȡsocket<%u>�Զ�����Ķ˿�ʧ�ܣ�", s);
			CloseSocket(s);
			return false;
		}
		Log("�Զ������׽��ֶ˿�<%d>", nPort);	
	}

	//�����׽���
	m_s = s;

	//���������߳�
	ResetEvent(m_hStop);
	m_hThread = AfxBeginThread(TH_Work, this, THREAD_PRIORITY_TIME_CRITICAL)->m_hThread;
	return true;
}

//ֹͣ
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
* �������ƣ�	
* ����������	ʹ���Զ������׽��ַ�������
* ���������	pData			-- �����͵�����ָ��
*				nLen			-- ���������ݵĳ���
* ���������	
* �� �� ֵ��	�������ݳɹ�����true�����򷵻�false��
* ����˵����	��������������ʹ�ý��յ��׽��ַ�������,���Ա����ڵ���start��
*				����stopǰ���ñ��ӿڡ�
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2014-12-03	�ű�	      	����
*******************************************************************************/
bool CUdpY::Send(const void *pData, int nLen)
{
	//Log("���ͣ�");
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
*��    ��:	�����Զ������׽��ֵ�Ŀ���ַ
*�������:	strIP			-- ���ݷ������ĸ�IP
*			nPort			-- ���ݷ������ĸ��˿�
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���		�޸�����
--------------------------------------------------------------------------------
2016-7-26	�ű�		����		
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
* �������ƣ�	
* ����������	�����ֶ������׽��ֵ�Ŀ�귢�͵�ַ		
* ���������	strIP			-- ���ݷ������ĸ�IP
*				nPort			-- ���ݷ������ĸ��˿�
*				strLocalIP		-- ��������ʹ�õı���IP
* ���������	
* �� �� ֵ��	ִ�гɹ�����true�����򷵻�false��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2014-12-03	�ű�	      	����
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

	//����socket
	SOCKET_HANDLE s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//�socket
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0 == nLocalPort ? 0 : htons(nLocalPort);
	addr.sin_addr.s_addr =  IP2Addr((LPCSTR(strLocalIP.GetBuffer(0))));
	if(0 != bind(s, (sockaddr *) &addr, sizeof(addr)))
	{
		Log("�˿ڵ�ַ��ʧ��<%s:%d>��", strLocalIP.GetBuffer(0), nPort);
		CloseSocket(s);
		return false;
	}
	//����Socket����(һ��Ĭ����8192)
	int nBuffSize = 8192 * 800;
	if(0 != setsockopt(s, SOL_SOCKET, SO_SNDBUF, (VAL_TYPE)&nBuffSize, sizeof(nBuffSize)))
	{
		Log("���ö˿ڻ����Сʧ��<%s:%d>��", strLocalIP, nPort);
	}

	m_sManual = s;
	return true;
}

/*******************************************************************************
* �������ƣ�	
* ����������	ʹ���ֶ������׽��ַ�������
* ���������	pData			-- �����͵�����ָ��
*				nLen			-- ���������ݵĳ���
* ���������	
* �� �� ֵ��	ִ�гɹ�����true�����򷵻�false��
* ����˵����	������Ӧ���ڵ���SetDstAddr�����
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2014-12-03	�ű�	      	����
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
*��    ��:	�����������ֶ������׽��ֵ�����
*�������:	pBuff			-- �������ݵĻ�����
*			nSize			-- ���ջ������Ĵ�С
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-12	�ű�			����		
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
		Log("�ֶ��������ݴ���nCount<%d> nErrCode<%d>", nCount, WSAGetLastError());
		return 0;
	}
	return nCount;
}

/*******************************************************************************
*��    ��:	��ȡ�Զ������׽��ֵĵ�ַ��Ϣ
*�������:	strLocalIP	-- ����IP
*			nLocalPort	-- ���ض˿�
*			strDstIP	-- Ŀ��IP
*			nDstPort	-- Ŀ�Ķ˿�
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���		�޸�����
--------------------------------------------------------------------------------
2016-7-28	�ű�		����		
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
*��    ��:	��ȡ�ֶ������׽��ֵĵ�ַ��Ϣ
*�������:	strLocalIP	-- ����IP
*			nLocalPort	-- ���ض˿�
*			strDstIP	-- Ŀ��IP
*			nDstPort	-- Ŀ�Ķ˿�
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���		�޸�����
--------------------------------------------------------------------------------
2016-7-28	�ű�		����		
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
* �������ƣ�	
* ����������	�������ݵ��̺߳���
* ���������	lpContext		-- ��������
* ���������	
* �� �� ֵ��	0��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2014-12-03	�ű�	      	����
*******************************************************************************/
UINT CUdpY::TH_Work(LPVOID lpContext)
{
	CUdpY *pThis = (CUdpY*)lpContext;

	//��ȡ�����ϵ�����
	char RecvBuf[10240] = {0};
	while(WAIT_TIMEOUT == WaitForSingleObject(pThis->m_hStop, 1))
	{
		//����Ƿ������ݵ���
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

		//�����ݵ���
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
			Log("���ݽ��մ���nCount<%d> nErrCode<%d>", nCount, WSAGetLastError());
			continue;
		}
		//Log("���յ�����<%d>", nCount);
	
		//�ص�֪ͨ
		if (0 != pThis->m_pfnData)
		{
			pThis->m_pfnData(pThis->m_lpData, RecvBuf, nCount,
				std::string(inet_ntoa(addrSender.sin_addr)), htons(addrSender.sin_port));
		}
	}
	pThis->m_hThread = NULL;
	return 0;
}

//IP��ַת������
unsigned long CUdpY::IP2Addr(const char * lpszIP)
{
	if (0 == lpszIP || std::string(lpszIP) == "")
	{
		return htonl(INADDR_ANY);
	}
	return inet_addr(lpszIP);
}

//�ر�socket
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

//��ȡ�׽��ֵİ󶨵�ַ
bool CUdpY::GetAddr(SOCKET_HANDLE s, std::string &strIP, int &nPort)
{
	sockaddr_in addr;
	int nAddrLen = sizeof(addr);
	int nRet = getsockname(s, (sockaddr*)&addr, &nAddrLen); 
	if (0 != nRet)
	{
		Log("��ȡ�׽���<%u>�ĵ�ַʧ�� Err<%d>", s, nRet);
		return false;
	}
	nPort = htons(addr.sin_port);
	strIP = inet_ntoa(addr.sin_addr);
	return true;
}

}
