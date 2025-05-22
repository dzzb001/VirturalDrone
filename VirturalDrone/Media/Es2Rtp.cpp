#include "StdAfx.h"
#include "Es2Rtp.h"
#include <vector>

#define Log LogN(8000)

namespace Media
{

//构造
CEs2Rtp::CEs2Rtp(void)
{
	m_pfnRtp = NULL;
	m_lpRtp = NULL;
	m_RtpHeader.sn = 1;
	ResetRtpHeader(m_RtpHeader);
	m_eEncodeType = H264;
}

//析构
CEs2Rtp::~CEs2Rtp(void)
{
}

//清空参数
void CEs2Rtp::Reset()
{
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffPPS.clear();
		m_buffSPS.clear();
	}
	m_buffRtp.clear();
	m_buffNalu.clear();
	ResetRtpHeader(m_RtpHeader);
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	输入ES
* 输入参数：	pEs				-- ES数据指针
*				nLen			-- ES数据长度
*				nDTS			-- ES数据的DTS
* 输出参数：	
* 返 回 值：	无。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
*******************************************************************************/
void CEs2Rtp::EsIn(BYTE *pEs, int nLen, UINT nDTS)
{
	/*static FILE* pFile = nullptr;

	if (!pFile)
	{
		pFile = fopen("es.data", "wb+");
		fwrite(pEs, nLen, 1, pFile);
	}else
		fwrite(pEs, nLen, 1, pFile);*/

	//找出所有的起始码
	std::vector<std::pair<int, int> > vecStart;
	for (int i = 0; i+2 < nLen; ++i)
	{
		//优先找00 00 00 01这种起始码
		if (i + 3 < nLen)
		{
			if (0 == pEs[i] && 0 == pEs[i + 1] && 0 == pEs[i + 2] && 1 == pEs[i + 3])
			{
				vecStart.push_back(std::make_pair(i, 4));
				i += 3;
				continue;
			}
		}

		//不是 00 00 00 01，就看下是不是00 00 01这种起始码
		if (0 == pEs[i] && 0 == pEs[i + 1] && 1 == pEs[i + 2])
		{
			vecStart.push_back(std::make_pair(i, 3));
			i += 2;
		}
	}

	//如果NAL单元缓冲中已经有数据了，则把第一个起始码前的数据放到缓冲
	if (m_buffNalu.size() > 0)
	{
		int nDataLen = vecStart.empty() ? nLen : vecStart[0].first;
		m_buffNalu.append(pEs, nDataLen);
	}
	for (size_t i = 0; i < vecStart.size(); ++i)
	{
		//把之前的NAL打包发送
		if (m_eEncodeType == H264)
		{
			Nalu(nDTS);	
		}
		else
		{
			Nalu_hevc(nDTS);
		}

		//收集新的NAL单元及其时间戳
		m_RtpHeader.timestamp = htonl(nDTS);
		int nStartPos = vecStart[i].first + vecStart[i].second;	//跳过起始码
		int nDataLen = i+1 >= vecStart.size() ? nLen-nStartPos : vecStart[i+1].first-nStartPos;
		m_buffNalu.append(pEs+nStartPos, nDataLen);
	}
}

//注册RTP包输出回调函数
void CEs2Rtp::RegRtp(CT_Rtp pfn, LPVOID lpContext )
{
	m_pfnRtp = pfn;
	m_lpRtp = lpContext;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	获取sps、pps
* 输入参数：	
* 输出参数：	sps				-- sps缓冲区
*				pps				-- pps缓冲区
* 返 回 值：	获取成功返回TRUE，否则返回FALSE。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
*******************************************************************************/
BOOL CEs2Rtp::GetSPPS(Tool::TBuff<BYTE> &sps, Tool::TBuff<BYTE> &pps )
{
	sps.clear();
	pps.clear();
	//CSingleLock lock(&m_lockSPPS);
	//lock.Lock();
	std::lock_guard<std::mutex> lock(m_lockSPPS);
	if (m_buffPPS.size() == 0 || m_buffSPS.size() == 0)
	{
		//sps和pps还没有收集完
		return FALSE;
	}
	sps.append((byte*)&m_buffSPS[0], m_buffSPS.size());
	pps.append((byte*)&m_buffPPS[0], m_buffPPS.size());
	return TRUE;
}

/*******************************************************************************
* 函数名称：	
* 功能描述：	将Nalu打包成RTP包
* 输入参数：	uNextDTS		-- 下一个NALU的DTS
* 输出参数：	
* 返 回 值：	无。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-19	张斌	      	创建
*******************************************************************************/
void CEs2Rtp::Nalu(UINT uNextDTS)
{
	if (m_buffNalu.size() <= 0)
	{
		//没有要添加的数据，或者没有Rtp包头
		return;
	}

	nal_unit_header nal;
	memcpy(&nal, &m_buffNalu[0], 1);

	//收集pps和sps
	if (7 == nal.nal_unit_type)
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffSPS.clear();
		m_buffSPS.append(&m_buffNalu[0], m_buffNalu.size());
	}
	else if (8 == nal.nal_unit_type)
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffPPS.clear();
		m_buffPPS.append(&m_buffNalu[0], m_buffNalu.size());
	}

	//BOOL bIFrameStart = FALSE;
	//if (nal.nal_unit_type > 0 && nal.nal_unit_type <= 5)
	//{
	//	bIFrameStart = IsIFrameStart(&m_buffNalu[0], m_buffNalu.size());
	//}	

	//限制Rtp包的长度小于等于1400字节
	size_t nMaxLen = 1400;

	//一个NAL单元封装为一个Rtp包
	if (m_buffNalu.size() + sizeof(RtpHeader) <= nMaxLen)
	{
		m_RtpHeader.sn = htons(htons(m_RtpHeader.sn)+1);
		m_RtpHeader.M = 0;
		if (htonl(m_RtpHeader.timestamp) != uNextDTS)
		{
			m_RtpHeader.M = 1;
		}
		m_buffRtp.clear();
		m_buffRtp.append((BYTE*)&m_RtpHeader, sizeof(RtpHeader));
		m_buffRtp.append(&m_buffNalu[0], m_buffNalu.size());	
		if (NULL != m_pfnRtp)
		{
			m_pfnRtp(
				m_lpRtp,
				&m_buffRtp[0],
				m_buffRtp.size(),

				m_buffNalu.size(),
				htonl(m_RtpHeader.timestamp)
				);
		}
		m_buffNalu.clear();
		return;
	}

	//一个NAL单元封装为多个Rtp包
	int nPayloadPerRtp = nMaxLen - sizeof(RtpHeader) - 2;
	FU_A_header FuHeader = {0};
	FuHeader.type = nal.nal_unit_type;
	nal.nal_unit_type = 28;						//修改为FU_A的类型
	int nPayloadLen = 0;
	for (size_t nCount = 1; nCount < m_buffNalu.size(); nCount+=nPayloadLen)
	{
		int nLeft = m_buffNalu.size() - nCount;
		nPayloadLen = nLeft > nPayloadPerRtp ? nPayloadPerRtp : nLeft;
		FuHeader.S = 1 == nCount ?  1 : 0;
		FuHeader.E = nCount+nPayloadLen >= m_buffNalu.size() ? 1 : 0;
		m_RtpHeader.sn = htons(htons(m_RtpHeader.sn)+1);
		m_RtpHeader.M = 0;
		if (0 != FuHeader.E && htonl(m_RtpHeader.timestamp) != uNextDTS)
		{
			m_RtpHeader.M = 1;
		}
		m_buffRtp.clear();
		m_buffRtp.append((BYTE*)&m_RtpHeader, sizeof(RtpHeader));
		m_buffRtp.append((BYTE*)&nal, 1);
		m_buffRtp.append((BYTE*)&FuHeader, 1);
		m_buffRtp.append(&m_buffNalu[nCount], nPayloadLen);
		if (NULL != m_pfnRtp)
		{
			m_pfnRtp(
				m_lpRtp,
				&m_buffRtp[0],
				m_buffRtp.size(),
				nPayloadLen + 2,
				htonl(m_RtpHeader.timestamp)
				);
		}
	}
	m_buffNalu.clear();
}

void CEs2Rtp::Nalu_hevc(UINT uNextDTS)
{
	if (m_buffNalu.size() <= 0)
	{
		//没有要添加的数据，或者没有Rtp包头
		return;
	}

	NALHeader_hevc nal;
	memcpy(&nal, &m_buffNalu[0], 2);

	uint8_t forbiddenZeroBit = (m_buffNalu[0] >> 7) & 0x01;
	uint8_t nalUnitType = (m_buffNalu[0] >> 1) & 0x3F;
	uint8_t nuhLayerId = (m_buffNalu[1] >> 1) & 0x3F;
	uint8_t nuhTemporalIdPlus1 = m_buffNalu[1] & 0x07;

	// 输出解析结果
	//Log(Tool::Debug, "NAL Unit Header:\nForbidden Zero Bit: %d\nNAL Unit Type: %d\nNUH Layer ID: %d\nNUH Temporal ID Plus1: %d\n", (int)forbiddenZeroBit, (int)nalUnitType, (int)nuhLayerId, (int)nuhTemporalIdPlus1);

	//收集pps和sps
	if (32 == nalUnitType)
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffVPS.clear();
		m_buffVPS.append(&m_buffNalu[0], m_buffNalu.size());
	}
	if (33 == nalUnitType)
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffSPS.clear();
		m_buffSPS.append(&m_buffNalu[0], m_buffNalu.size());
	}
	else if (34 == nalUnitType)
	{
		std::lock_guard<std::mutex> lock(m_lockSPPS);
		m_buffPPS.clear();
		m_buffPPS.append(&m_buffNalu[0], m_buffNalu.size());
	}
	else if (39 == nalUnitType)
	{
		//SEI(补充增强信息)
	}

	//限制Rtp包的长度小于等于1400字节
	size_t nMaxLen = 1400;
	m_RtpHeader.PT = 99;
	//一个NAL单元封装为一个Rtp包
	if (m_buffNalu.size() + sizeof(RtpHeader) <= nMaxLen)
	{
		m_RtpHeader.sn = htons(htons(m_RtpHeader.sn) + 1);
		m_RtpHeader.M = (htonl(m_RtpHeader.timestamp) != uNextDTS) ? 1 : 0;

		m_buffRtp.clear();
		m_buffRtp.append((BYTE*)&m_RtpHeader, sizeof(RtpHeader));
		m_buffRtp.append(&m_buffNalu[0], m_buffNalu.size());
		if (NULL != m_pfnRtp)
		{
			m_pfnRtp(
				m_lpRtp,
				&m_buffRtp[0],
				m_buffRtp.size(),
				m_buffNalu.size(),
				htonl(m_RtpHeader.timestamp)
			);
		}
		m_buffNalu.clear();
		return;
	}

	//一个NAL单元封装为多个Rtp包
	int nPayloadPerRtp = nMaxLen - sizeof(RtpHeader) - 3; //预留的数据
	FU_A_h265_header FuHeader = { 0 };

	BYTE* pNal = (BYTE*)&nal;
	pNal[0] = 49 << 1;
	pNal[1] = 1;

	FuHeader.type = nalUnitType;
	//Log(Tool::Debug, "pNal[0]:%x, pNal[1]:%x，FuHeader.type:%d", pNal[0], pNal[1], FuHeader.type);
	size_t nCount = 2;
	while (nCount < m_buffNalu.size())
	{
		int nLeft = m_buffNalu.size() - nCount;
		int nPayloadLen = (nLeft > nPayloadPerRtp) ? nPayloadPerRtp : nLeft;

		FuHeader.S = (nCount == 2) ? 1 : 0;
		FuHeader.E = (nCount + nPayloadLen >= m_buffNalu.size()) ? 1 : 0;

		uint8_t* pThd = (uint8_t*)&FuHeader;
		if (FuHeader.S)
		{
			pThd[0] |= 1 << 7;
			//Log(Tool::Debug, "fuhead.s:1, pThd %d, type %d", *pThd, FuHeader.type);
		}
		else if (FuHeader.E)
		{
			pThd[0] &= ~(1 << 7);
			//Log(Tool::Debug, "fuhead.e:1, pThd %d, type %d", *pThd, FuHeader.type);
		}
		else
		{
			pThd[0] = nalUnitType;
			//Log(Tool::Debug, "fuhead.s,e:0,0, pThd %d, type %d", *pThd,FuHeader.type);
		}

		m_RtpHeader.sn = htons(htons(m_RtpHeader.sn) + 1);
		m_RtpHeader.M = (FuHeader.E && htonl(m_RtpHeader.timestamp) != uNextDTS) ? 1 : 0;

		m_buffRtp.clear();
		m_buffRtp.append(reinterpret_cast<const BYTE*>(&m_RtpHeader), sizeof(RtpHeader));
		m_buffRtp.append(pNal, 2);
		m_buffRtp.append(/*reinterpret_cast<const BYTE*>(&FuHeader)*/pThd, 1);
		m_buffRtp.append(&m_buffNalu[nCount], nPayloadLen);

		//Log(Tool::Debug, "**********FuHeader.type:%d", FuHeader.type);
		if (m_pfnRtp != nullptr)
		{
			m_pfnRtp(m_lpRtp, &m_buffRtp[0], m_buffRtp.size(), nPayloadLen + 3, htonl(m_RtpHeader.timestamp));
		}

		nCount += nPayloadLen;
	}
	m_buffNalu.clear();
}
/*******************************************************************************
* 函数名称：	
* 功能描述：	判断一个NAL单元是否是I帧的开始
* 输入参数：	pBuff			-- NAL单元的数据指针，不包括起始码
*				nLen			-- 数据长度
* 输出参数：	
* 返 回 值：	执行成功返回TRUE，否则返回FALSE。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2013-07-29	张斌	      	创建
*******************************************************************************/
BOOL CEs2Rtp::IsIFrameStart(BYTE *pBuff, UINT nLen)
{
	UINT nBit = 0;
	UINT first_mb_in_slice = GetUeValue(pBuff+1, nLen-1, nBit);
	UINT slice_type = GetUeValue(pBuff+1, nLen-1, nBit);
	if (0 == first_mb_in_slice && (2 == slice_type || 7 == slice_type))
	{
		return TRUE;
	}
	
	return FALSE;
}

/*******************************************************************************
* 函数名称：	GetUeValue
* 功能描述：	从缓冲区解码无符号哥伦布编码值；
* 输入参数：	pBuff		-- 待解码的缓冲区；
*				nLen		-- 输入数据的字节长度；
*				nStartBit	-- 待解码数据相对输入缓冲区开头的起始bit位；
* 输出参数：	nStartBit	-- 该哥伦布编码值之后的第一个bit位；
* 返 回 值：	返回解码出的哥伦布编码值。
* 其它说明：	
* 修改日期		修改人	      修改内容
* ------------------------------------------------------------------------------
* 2010-07-19	周锋	      创建
*******************************************************************************/
UINT CEs2Rtp::GetUeValue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算的个数
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;

	//计算值
	DWORD dwRet = 0, dwMask = 1;
	for (UINT i=0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (0x1 << nZeroNum) - 1 + dwRet;
}

void CEs2Rtp::ResetRtpHeader(RtpHeader &header)
{
	header.version = 2;
	header.P = 0;
	header.X = 0;
	header.CC = 0;
	header.M = 0;
	//header.PT = 96;
	header.PT = 98;
	//header.sn = 1;
	header.timestamp = 0;
	header.SSRC = 0xEEEEEEEE;
}
}

