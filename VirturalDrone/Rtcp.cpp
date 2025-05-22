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
	//Rtcp包头
	struct RtcpHeader 
	{
		unsigned char SC		: 5;		//接收者报告块个数
		unsigned char P		: 1;		//填充标记位
		unsigned char V		: 2;		//协议版本
		unsigned char PT;				//包类型(SR = 200, RR = 201, SD = 202)
		unsigned short length;			//包长度(单位为4字节，为本字段之后本包剩余数据的长度)
		unsigned int ssrc;				//同步源标识符
	};

	//发送者报告
	struct SR 
	{
		RtcpHeader header;

		//发送者信息
		unsigned int ntp_1;
		unsigned int ntp_2;
		unsigned int time;				//Rtp时间戳
		unsigned int pack_count;		//发送包个数
		unsigned int byte_count;		//发送字节数

		//以下为SC个接收报告块
	};	

	//描述信息
	struct SD 
	{
		RtcpHeader header;

		unsigned char type;				//1表示CName(user and domain)
		unsigned char len;				//描述信息长度

		//描述信息(字符串)

		//填充的0，保证整个SD的长度为4的整数倍
	};

	//Rtp头
	struct RtpHeader
	{
		unsigned char CC : 4;		//CSRC的个数
		unsigned char   X : 1;		//扩展标志位
		unsigned char   P : 1;		//填充标志位
		unsigned char   version : 2;		//版本号

		unsigned char   PT : 7;		//负载类型
		unsigned char   M : 1;		//标志位(由具体协议确定)

		unsigned short sn;						//序列号
		unsigned int timestamp;				//时间戳
		unsigned int SSRC;						//同步源识别标识符
	};


#pragma pack()
}


//构造
CRtcp::CRtcp()
{
	Reset();
}

//析构
CRtcp::~CRtcp(void)
{	
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取要发送的RTCP包
* 输入参数：	nInterLeavedCh	-- RTCP交织通道号，-1表示不需要使用交织包
* 输出参数：	buffRtcp		-- 要发送的RTCP数据
* 返 回 值：	有要发送的RTCP包返回TRUE，否则返回FALSE。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
*******************************************************************************/
bool CRtcp::GetRtcp(Tool::TBuff<unsigned char> &buffRtcp, bool bBye, int nInterLeavedCh /* = -1 */)
{
	unsigned long dwNow = CComTime::GetTickCount();
	if (!bBye && (!m_bInitTime || dwNow - m_dwLastSendRtcp < m_dwRtcpInterval))
	{
		//还没有发送过Rtp包或者还没到发送时间
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

	//NTP(timeval_t *tv)；
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
		//加入交织包头
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
* 函数名称：	
* 功能描述：	Rtp包发送通知,用于Rtcp信息的统计
* 输入参数：	pRtp			-- Rtp包指针
*				nLen			-- Rtp长度
*				nPayloadLen		-- Rtp包负载字节数
* 输出参数：	
* 返 回 值：	无。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
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

	//还未初始化时间或者时间跳跃大于1分钟，初始化时间参数
	if (!m_bInitTime || dwRtpTime - m_dwLastRtpTime > 5400000)
	{
		//Log("RTCP 初始化时间");
		InitTime(dwRtpTime);
	}
	m_dwLastRtpTime = dwRtpTime;
}

//Rtcp包输入
void CRtcp::RtcpIn(unsigned char *pRtcp, int nLen )
{
	//不需要做处理
}

//重置状态
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
	int nZeroCount = wLen * 4 - 4 - 2 - strDes.size(); //为保证4字节整数倍，需要的填充字节数
	m_buffRtcp.append((unsigned char*)&packSD, sizeof(SD));
	m_buffRtcp.append((unsigned char*)(&strDes[0]), strDes.size());
	for (int i=0; i<nZeroCount; i++)
	{
		m_buffRtcp.append(0);
	}
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	初始化时间参数，包括NTP和RTP时间，及发送RTCP时间间隔控制
* 输入参数：	dwRtpTime		-- 当前的RTP时间
* 输出参数：	
* 返 回 值：	无。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
*******************************************************************************/
void CRtcp::InitTime(unsigned int dwRtpTime)
{
#if 0
	LARGE_INTEGER nFreq;
	LARGE_INTEGER nNow;
	QueryPerformanceCounter(&nNow);
	QueryPerformanceFrequency(&nFreq);

	//NTP时间相关
	m_nNTPFreq = nFreq.QuadPart;
	timeb timeNow;
	ftime(&timeNow);
	m_nNTPOffSet = timeNow.time*m_nNTPFreq + (unsigned int(timeNow.millitm)*m_nNTPFreq)/1000 - nNow.QuadPart;	

	//RTP时间相关
	m_dwRtpTimeOffSet = dwRtpTime - (unsigned int)(timeNow.time*m_dwRtpTimeFreq + (timeNow.millitm/1000.0)*m_dwRtpTimeFreq + 0.5);
#endif
	//RTCP发送间隔控制相关
	m_dwRtcpInterval = GetRandom();	
	m_dwLastSendRtcp = CComTime::GetTickCount();

	//设置标识
	m_bInitTime = true;
	return;	
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取NTP、RTP时间
* 输入参数：	
* 输出参数：	dwNIP32_63		-- NTP时间的高位
*				dwNTP0_31		-- NTP时间的低位
*				dwRtpTime		-- RTP时间
* 返 回 值：	执行成功返回TRUE，否则返回FALSE。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-22	张斌	      	创建
*******************************************************************************/
//bool CRtcp::GetTime(unsigned int &dwNTP32_63, unsigned int &dwNTP0_31, unsigned int &dwRtpTime)
//{
//	if (!m_bInitTime)
//	{
//		return false;
//	}
//	CComTime::timeval_t timeNow;
//	gettimeofday(&timeNow);
//	dwNTP32_63 = timeNow.tv_sec + 0x83AA7E80;//换算时间起点
//	double fFractionalPart = (timeNow.tv_usec/15625.0)*0x04000000; // 2^32/10^6 换算单位
//	dwNTP0_31 = (unsigned int)(fFractionalPart + 0.5);
//	dwRtpTime = (unsigned int)(timeNow.tv_sec*m_dwRtpTimeFreq + (timeNow.tv_usec/1000000.0)*m_dwRtpTimeFreq + 0.5 + m_dwRtpTimeOffSet);
//	return TRUE;
//}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取当前时间
* 输入参数：	
* 输出参数：	pTime			-- 当前时间
* 返 回 值：	执行成功返回TRUE，否则返回false。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-22	张斌	      	创建
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

//生成2500至7500的随机数
unsigned int CRtcp::GetRandom()
{
	if (!m_bInitRandom)
	{
		srand(CComTime::GetTickCount());
		m_bInitRandom = true;
	}
	return (unsigned int)(((double)rand()) / (RAND_MAX + 1) * (7500 - 2500) + 2500);
}

