#pragma once
#include "../Tool/TBuff.h"
#include "Parser.h"
#include <mutex>

namespace Media
{

//将ES打包成RTP包的类
class CEs2Rtp
{
public:
	//构造
	CEs2Rtp(void);

	//析构
	~CEs2Rtp(void);

	//编码类型
	enum eEncodeType
	{
		H264 = 0,
		H265
	} m_eEncodeType;
	//清空参数
	void Reset();

	//输入ES
	void EsIn(BYTE *pEs, int nLen, UINT nDTS);

	//设置编码数据的类型
	void SetEncodeType(eEncodeType type) { m_eEncodeType = type; }

	typedef void (*CT_Rtp)(
		LPVOID lpContext,
		byte *pRtp,
		int nLen,
		int nPayloadLen,
		UINT nDTS
		);

	//注册RTP包输出回调函数
	void RegRtp(
		CT_Rtp pfn,			//视频Rtp包输出回调函数指针
		LPVOID lpContext		//回调函数环境变量
		);

	//获取sps、pps
	BOOL GetSPPS(
		Tool::TBuff<BYTE> &sps,	//输出，sps
		Tool::TBuff<BYTE> &pps	//输出，pps
		);

protected:

#pragma pack(1)

	//Rtp头
	struct RtpHeader
	{
		byte CC				: 4;		//CSRC的个数
		byte X				: 1;		//扩展标志位
		byte P				: 1;		//填充标志位
		byte version		: 2;		//版本号

		byte PT				: 7;		//负载类型
		byte M				: 1;		//标志位(由具体协议确定)

		WORD sn;						//序列号
		DWORD timestamp;				//时间戳
		DWORD SSRC;						//同步源识别标识符
	};

	//NAL unit的头
	struct nal_unit_header 
	{
		BYTE nal_unit_type				: 5;
		BYTE nal_ref_idc				: 2;
		BYTE forbidden_zero_bit			: 1;
	};

	//FU-A头（FU-A分片传输的分片头）
	struct  FU_A_header
	{
		byte type		: 5;			//nal类型
		byte R			: 1;			//必须为0
		byte E			: 1;			//结束标记位
		byte S			: 1;			//开始标记位
	};

	//FU-A头（FU-A分片传输的分片头）
	struct  FU_A_h265_header
	{
		byte type : 6;			//nal类型
		//byte R : 1;			//必须为0
		byte E : 1;			//结束标记位
		byte S : 1;			//开始标记位
	};

	//hevc
	typedef struct {
		uint8_t forbidden_zero_bit; // 1 bit
		uint8_t nal_unit_type;      // 6 bits
		uint8_t nuh_layer_id;       // 6 bits
		uint8_t nuh_temporal_id_plus1; // 3 bits
	} NALHeader_hevc;
#pragma pack()

	//存储sps、pps NAL单元的缓冲区
	Tool::TBuff<BYTE> m_buffSPS;
	Tool::TBuff<BYTE> m_buffPPS;

	//VPS h265 增加的参数集
	Tool::TBuff<BYTE> m_buffVPS;
	//sps、pps的NAL单元缓冲区访问保护
	//CCriticalSection m_lockSPPS;
	std::mutex		   m_lockSPPS;

	//存储一个Rtp包
	Tool::TBuff<BYTE> m_buffRtp;

	//存储一个Nalu单元
	Tool::TBuff<BYTE> m_buffNalu;

	//Rtp包头
	RtpHeader m_RtpHeader;

	//RTP输出回调函数及环境变量
	CT_Rtp m_pfnRtp;
	LPVOID m_lpRtp;

protected:

	//h264 将Nalu打包成RTP包
	void Nalu(UINT uNextDTS);

	//h265数据流，将Nalu打包成RTP包
	void Nalu_hevc(UINT uNextDTS);

	BOOL IsIFrameStart(BYTE *pBuff, UINT nLen);

	//从缓冲区解码无符号哥伦布编码值
	UINT GetUeValue(BYTE *pBuff, UINT nLen, UINT &nStartBit);

	//清空Rtp头信息
	void ResetRtpHeader(RtpHeader &header);

};

}
