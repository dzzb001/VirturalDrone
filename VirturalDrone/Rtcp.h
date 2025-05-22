#pragma once
#include "Tool/TBuff.h"
#include "Tool/ComTime.h"

//Rtcp协议的处理类
class CRtcp
{

public:

	//构造
	CRtcp();

	//析构
	~CRtcp(void);

	//获取要发送的Rtcp包，如果没有要发送的包，返回FALSE
	bool GetRtcp(Tool::TBuff<unsigned char> &buffRtcp, bool bBye, int nInterLeavedCh = -1);

	//Rtp包发送通知,用于Rtcp信息的统计
	void RtpIn(
		unsigned char *pRtp,			//Rtp包指针
		int nLen,			//Rtp长度
		int nPayloadLen		//Rtp包负载字节数
		);

	//Rtcp包输入
	void RtcpIn(
		unsigned char *pRtcp,		//Rtcp包指针
		int nLen			//Rtcp包长度
		);

	//重置状态
	void Reset();


protected:

	//RTCP包缓冲区
	Tool::TBuff<unsigned char> m_buffRtcp;

	//发送的Rtp包计数
	unsigned long m_dwRtpCount;

	//发送的Rtp负载计数（字节）
	unsigned long m_dwPayloadCount;	

	//同步源标识
	unsigned long m_dwSSRC;

	//用于计算时间
	unsigned long long m_nNTPOffSet;		//NTP时间与CPU时间的差值
	unsigned long long m_nNTPFreq;			//NTP时钟频率
	int m_bInitTime;			//是否初始化过时间参数了
	unsigned long m_dwRtpTimeFreq;		//RTP的时钟频率
	unsigned long m_dwRtpTimeOffSet;	//RTP与UTC时间的差值

	//上次收到的RTP包的时间戳
	unsigned long m_dwLastRtpTime;

	//下次发送Rtcp包的间隔时间
	unsigned long m_dwRtcpInterval;

	//最后一次发送RTCP包的时间
	unsigned long m_dwLastSendRtcp;

	//是否初始化过随机数了
	bool m_bInitRandom;

	//上次收到RTCP包的时间

protected:

	//初始化时间参数，包括NTP和RTP时间，及发送RTCP时间间隔等
	void InitTime(unsigned int dwRtpTime);

	//获取NTP、RTP时间
	//bool GetTime(unsigned int &dwNTP32_63, unsigned int &dwNTP0_31, unsigned int &dwRtpTime);

	//获取当前时间
	//bool gettimeofday(struct CComTime::timeval_t* tp);

	//生成2500至7500的随机数
	unsigned int GetRandom();

};
