#pragma once
#include "Tool/TBuff.h"
#include "Tool/ComTime.h"

//RtcpЭ��Ĵ�����
class CRtcp
{

public:

	//����
	CRtcp();

	//����
	~CRtcp(void);

	//��ȡҪ���͵�Rtcp�������û��Ҫ���͵İ�������FALSE
	bool GetRtcp(Tool::TBuff<unsigned char> &buffRtcp, bool bBye, int nInterLeavedCh = -1);

	//Rtp������֪ͨ,����Rtcp��Ϣ��ͳ��
	void RtpIn(
		unsigned char *pRtp,			//Rtp��ָ��
		int nLen,			//Rtp����
		int nPayloadLen		//Rtp�������ֽ���
		);

	//Rtcp������
	void RtcpIn(
		unsigned char *pRtcp,		//Rtcp��ָ��
		int nLen			//Rtcp������
		);

	//����״̬
	void Reset();


protected:

	//RTCP��������
	Tool::TBuff<unsigned char> m_buffRtcp;

	//���͵�Rtp������
	unsigned long m_dwRtpCount;

	//���͵�Rtp���ؼ������ֽڣ�
	unsigned long m_dwPayloadCount;	

	//ͬ��Դ��ʶ
	unsigned long m_dwSSRC;

	//���ڼ���ʱ��
	unsigned long long m_nNTPOffSet;		//NTPʱ����CPUʱ��Ĳ�ֵ
	unsigned long long m_nNTPFreq;			//NTPʱ��Ƶ��
	int m_bInitTime;			//�Ƿ��ʼ����ʱ�������
	unsigned long m_dwRtpTimeFreq;		//RTP��ʱ��Ƶ��
	unsigned long m_dwRtpTimeOffSet;	//RTP��UTCʱ��Ĳ�ֵ

	//�ϴ��յ���RTP����ʱ���
	unsigned long m_dwLastRtpTime;

	//�´η���Rtcp���ļ��ʱ��
	unsigned long m_dwRtcpInterval;

	//���һ�η���RTCP����ʱ��
	unsigned long m_dwLastSendRtcp;

	//�Ƿ��ʼ�����������
	bool m_bInitRandom;

	//�ϴ��յ�RTCP����ʱ��

protected:

	//��ʼ��ʱ�����������NTP��RTPʱ�䣬������RTCPʱ������
	void InitTime(unsigned int dwRtpTime);

	//��ȡNTP��RTPʱ��
	//bool GetTime(unsigned int &dwNTP32_63, unsigned int &dwNTP0_31, unsigned int &dwRtpTime);

	//��ȡ��ǰʱ��
	//bool gettimeofday(struct CComTime::timeval_t* tp);

	//����2500��7500�������
	unsigned int GetRandom();

};
