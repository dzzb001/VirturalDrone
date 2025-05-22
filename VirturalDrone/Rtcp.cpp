#include "stdafx.h"
#include "Rtcp.h"
#include <sys/timeb.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#define Log LogN(3000)

namespace
{

#pragma pack(1)	
	//Rtcp��ͷ
	struct RtcpHeader 
	{
		unsigned char SC		: 5;		//�����߱�������
		unsigned char P		: 1;		//�����λ
		unsigned char V		: 2;		//Э��汾
		unsigned char PT;				//������(SR = 200, RR = 201, SD = 202)
		unsigned short length;			//������(��λΪ4�ֽڣ�Ϊ���ֶ�֮�󱾰�ʣ�����ݵĳ���)
		unsigned int ssrc;				//ͬ��Դ��ʶ��
	};

	//�����߱���
	struct SR 
	{
		RtcpHeader header;

		//��������Ϣ
		unsigned int ntp_1;
		unsigned int ntp_2;
		unsigned int time;				//Rtpʱ���
		unsigned int pack_count;		//���Ͱ�����
		unsigned int byte_count;		//�����ֽ���

		//����ΪSC�����ձ����
	};	

	//������Ϣ
	struct SD 
	{
		RtcpHeader header;

		unsigned char type;				//1��ʾCName(user and domain)
		unsigned char len;				//������Ϣ����

		//������Ϣ(�ַ���)

		//����0����֤����SD�ĳ���Ϊ4��������
	};

	//Rtpͷ
	struct RtpHeader
	{
		unsigned char CC : 4;		//CSRC�ĸ���
		unsigned char   X : 1;		//��չ��־λ
		unsigned char   P : 1;		//����־λ
		unsigned char   version : 2;		//�汾��

		unsigned char   PT : 7;		//��������
		unsigned char   M : 1;		//��־λ(�ɾ���Э��ȷ��)

		unsigned short sn;						//���к�
		unsigned int timestamp;				//ʱ���
		unsigned int SSRC;						//ͬ��Դʶ���ʶ��
	};


#pragma pack()
}


//����
CRtcp::CRtcp()
{
	Reset();
}

//����
CRtcp::~CRtcp(void)
{	
}

/*******************************************************************************
* �������ƣ�	
* ����������	��ȡҪ���͵�RTCP��
* ���������	nInterLeavedCh	-- RTCP��֯ͨ���ţ�-1��ʾ����Ҫʹ�ý�֯��
* ���������	buffRtcp		-- Ҫ���͵�RTCP����
* �� �� ֵ��	��Ҫ���͵�RTCP������TRUE�����򷵻�FALSE��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-19	�ű�	      	����
*******************************************************************************/
bool CRtcp::GetRtcp(Tool::TBuff<unsigned char> &buffRtcp, bool bBye, int nInterLeavedCh /* = -1 */)
{
	unsigned long dwNow = CComTime::GetTickCount();
	if (!bBye && (!m_bInitTime || dwNow - m_dwLastSendRtcp < m_dwRtcpInterval))
	{
		//��û�з��͹�Rtp�����߻�û������ʱ��
		return false;
	}	
	
	SR *pSR = (SR*)&m_buffRtcp[0];
	pSR->header.ssrc = m_dwSSRC;
	unsigned int dwNTP1, dwNTP2, dwRtpTime;

	CComTime::timeval_t ntp;
	CComTime::NTP(&ntp);
	pSR->ntp_1 = htonl((unsigned int)ntp.tv_sec);
	pSR->ntp_2 = htonl((unsigned int)ntp.tv_usec);
	//GetTime(dwNTP1, dwNTP2, dwRtpTime);

	//NTP(timeval_t *tv)��
	//pSR->ntp_1 = htonl(dwNTP1);
	//pSR->ntp_2 = htonl(dwNTP2);
	dwRtpTime = m_dwLastRtpTime;
	pSR->time = htonl(dwRtpTime);
	pSR->pack_count = htonl(m_dwRtpCount);
	pSR->byte_count = htonl(m_dwPayloadCount);

	if (bBye)
	{
		size_t nStart = m_buffRtcp.size();
		m_buffRtcp.resize(m_buffRtcp.size() + sizeof(RtcpHeader));
		RtcpHeader *pHeader = (RtcpHeader*)(&m_buffRtcp[nStart]);
		pHeader->V = 2;
		pHeader->P = 0;
		pHeader->SC = 1;
		pHeader->PT = 203;
		pHeader->length = htons(1);
		pHeader->ssrc = m_dwSSRC;
	}
	else
	{
		SD *pSD = (SD*)&m_buffRtcp[sizeof(SR)];
		pSD->header.ssrc = m_dwSSRC;
	}


	buffRtcp.clear();
	if (-1 != nInterLeavedCh)
	{
		//���뽻֯��ͷ
		buffRtcp.append(0x24);
		buffRtcp.append((unsigned char)nInterLeavedCh);
		unsigned short wPayloadLen = htons((unsigned short)m_buffRtcp.size());
		buffRtcp.append((unsigned char*)&wPayloadLen, 2);		
	}
	buffRtcp.append(&m_buffRtcp[0], m_buffRtcp.size());

	m_dwRtcpInterval = GetRandom();
	m_dwLastSendRtcp = dwNow;	
	return true;
}

/*******************************************************************************
* �������ƣ�	
* ����������	Rtp������֪ͨ,����Rtcp��Ϣ��ͳ��
* ���������	pRtp			-- Rtp��ָ��
*				nLen			-- Rtp����
*				nPayloadLen		-- Rtp�������ֽ���
* ���������	
* �� �� ֵ��	�ޡ�
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-19	�ű�	      	����
*******************************************************************************/
void CRtcp::RtpIn(unsigned char *pRtp, int nLen, int nPayloadLen )
{
	unsigned long dwSSRC = *((unsigned long*)(pRtp + 8));
	if (m_dwSSRC != dwSSRC)
	{
		m_dwRtpCount = 0;
		m_dwPayloadCount = 0;
		m_dwSSRC = dwSSRC;
	}
	m_dwRtpCount ++;
	m_dwPayloadCount += nPayloadLen;

	unsigned long dwRtpTime = htonl(*(unsigned long*)(pRtp+4));

	//��δ��ʼ��ʱ�����ʱ����Ծ����1���ӣ���ʼ��ʱ�����
	if (!m_bInitTime || dwRtpTime - m_dwLastRtpTime > 5400000)
	{
		//Log("RTCP ��ʼ��ʱ��");
		InitTime(dwRtpTime);
	}
	m_dwLastRtpTime = dwRtpTime;
}

//Rtcp������
void CRtcp::RtcpIn(unsigned char *pRtcp, int nLen )
{
	//����Ҫ������
}

//����״̬
void CRtcp::Reset()
{
	m_dwRtpCount = 0;
	m_dwPayloadCount  = 0;
	m_dwSSRC = 0;
	m_bInitTime = false;
	m_nNTPFreq = 0;
	m_nNTPOffSet = 0;
	m_dwRtpTimeFreq = 90000;
	m_dwRtpTimeOffSet = 0;
	m_bInitRandom = false;
	m_dwRtcpInterval = 0;
	m_dwLastSendRtcp = 0;
	m_dwLastRtpTime = 0;

	m_buffRtcp.clear();
	SR packSR;
	packSR.header.V = 2;
	packSR.header.P = 0;
	packSR.header.SC = 0;
	packSR.header.PT = 200;
	unsigned short nsLen=6;
	packSR.header.length = htons(nsLen);
	m_buffRtcp.append((unsigned char*)&packSR, sizeof(packSR));

	SD packSD;
	std::string strDes = "SERVER";
	packSD.header.V = 2;
	packSD.header.P = 0;
	packSD.header.SC = 1;
	packSD.header.PT = 202;
	unsigned short wLen = (4 + 2 + strDes.size() +3)/4;
	packSD.header.length = htons(wLen);
	packSD.type = 1;
	packSD.len = strDes.size();
	int nZeroCount = wLen * 4 - 4 - 2 - strDes.size(); //Ϊ��֤4�ֽ�����������Ҫ������ֽ���
	m_buffRtcp.append((unsigned char*)&packSD, sizeof(SD));
	m_buffRtcp.append((unsigned char*)(&strDes[0]), strDes.size());
	for (int i=0; i<nZeroCount; i++)
	{
		m_buffRtcp.append(0);
	}
}

/*******************************************************************************
* �������ƣ�	
* ����������	��ʼ��ʱ�����������NTP��RTPʱ�䣬������RTCPʱ��������
* ���������	dwRtpTime		-- ��ǰ��RTPʱ��
* ���������	
* �� �� ֵ��	�ޡ�
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-19	�ű�	      	����
*******************************************************************************/
void CRtcp::InitTime(unsigned int dwRtpTime)
{
#if 0
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nNow;
	QueryPerformanceCounter(&nNow);
	QueryPerformanceFrequency(&nFreq);

	//NTPʱ�����
	m_nNTPFreq = nFreq.QuadPart;
	timeb timeNow;
	ftime(&timeNow);
	m_nNTPOffSet = timeNow.time*m_nNTPFreq + (unsigned int(timeNow.millitm)*m_nNTPFreq)/1000 - nNow.QuadPart;	

	//RTPʱ�����
	m_dwRtpTimeOffSet = dwRtpTime - (unsigned int)(timeNow.time*m_dwRtpTimeFreq + (timeNow.millitm/1000.0)*m_dwRtpTimeFreq + 0.5);
#endif
	//RTCP���ͼ���������
	m_dwRtcpInterval = GetRandom();	
	m_dwLastSendRtcp = CComTime::GetTickCount();

	//���ñ�ʶ
	m_bInitTime = true;
	return;	
}

/*******************************************************************************
* �������ƣ�	
* ����������	��ȡNTP��RTPʱ��
* ���������	
* ���������	dwNIP32_63		-- NTPʱ��ĸ�λ
*				dwNTP0_31		-- NTPʱ��ĵ�λ
*				dwRtpTime		-- RTPʱ��
* �� �� ֵ��	ִ�гɹ�����TRUE�����򷵻�FALSE��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-22	�ű�	      	����
*******************************************************************************/
//bool CRtcp::GetTime(unsigned int &dwNTP32_63, unsigned int &dwNTP0_31, unsigned int &dwRtpTime)
//{
//	if (!m_bInitTime)
//	{
//		return false;
//	}
//	CComTime::timeval_t timeNow;
//	gettimeofday(&timeNow);
//	dwNTP32_63 = timeNow.tv_sec + 0x83AA7E80;//����ʱ�����
//	double fFractionalPart = (timeNow.tv_usec/15625.0)*0x04000000; // 2^32/10^6 ���㵥λ
//	dwNTP0_31 = (unsigned int)(fFractionalPart + 0.5);
//	dwRtpTime = (unsigned int)(timeNow.tv_sec*m_dwRtpTimeFreq + (timeNow.tv_usec/1000000.0)*m_dwRtpTimeFreq + 0.5 + m_dwRtpTimeOffSet);
//	return TRUE;
//}

/*******************************************************************************
* �������ƣ�	
* ����������	��ȡ��ǰʱ��
* ���������	
* ���������	pTime			-- ��ǰʱ��
* �� �� ֵ��	ִ�гɹ�����TRUE�����򷵻�false��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-22	�ű�	      	����
*******************************************************************************/
//bool CRtcp::gettimeofday(struct CComTime::timeval_t* pTime)
// {
//	if (!m_bInitTime)
//	{
//		return false;
//
//	}
//	CComTime::GetTimeOfDay(pTime);
//
//	return true;
//
//	LARGE_INTEGER nNow;
//	QueryPerformanceCounter(&nNow);
//	nNow.QuadPart += m_nNTPOffSet;
//	pTime->tv_sec =  (long)(nNow.QuadPart / m_nNTPFreq);
//	pTime->tv_usec = (long)(((nNow.QuadPart % m_nNTPFreq) * 1000000L) / m_nNTPFreq);
//	return TRUE;
//}

//����2500��7500�������
unsigned int CRtcp::GetRandom()
{
	if (!m_bInitRandom)
	{
		srand(CComTime::GetTickCount());
		m_bInitRandom = true;
	}
	return (unsigned int)(((double)rand()) / (RAND_MAX + 1) * (7500 - 2500) + 2500);
}

