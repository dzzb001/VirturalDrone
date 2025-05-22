#pragma once
#include "../Tool/TBuff.h"
#include "Parser.h"
#include <mutex>

namespace Media
{

//��ES�����RTP������
class CEs2Rtp
{
public:
	//����
	CEs2Rtp(void);

	//����
	~CEs2Rtp(void);

	//��������
	enum eEncodeType
	{
		H264 = 0,
		H265
	} m_eEncodeType;
	//��ղ���
	void Reset();

	//����ES
	void EsIn(BYTE *pEs, int nLen, UINT nDTS);

	//���ñ������ݵ�����
	void SetEncodeType(eEncodeType type) { m_eEncodeType = type; }

	typedef void (*CT_Rtp)(
		LPVOID lpContext,
		byte *pRtp,
		int nLen,
		int nPayloadLen,
		UINT nDTS
		);

	//ע��RTP������ص�����
	void RegRtp(
		CT_Rtp pfn,			//��ƵRtp������ص�����ָ��
		LPVOID lpContext		//�ص�������������
		);

	//��ȡsps��pps
	BOOL GetSPPS(
		Tool::TBuff<BYTE> &sps,	//�����sps
		Tool::TBuff<BYTE> &pps	//�����pps
		);

protected:

#pragma pack(1)

	//Rtpͷ
	struct RtpHeader
	{
		byte CC				: 4;		//CSRC�ĸ���
		byte X				: 1;		//��չ��־λ
		byte P				: 1;		//����־λ
		byte version		: 2;		//�汾��

		byte PT				: 7;		//��������
		byte M				: 1;		//��־λ(�ɾ���Э��ȷ��)

		WORD sn;						//���к�
		DWORD timestamp;				//ʱ���
		DWORD SSRC;						//ͬ��Դʶ���ʶ��
	};

	//NAL unit��ͷ
	struct nal_unit_header 
	{
		BYTE nal_unit_type				: 5;
		BYTE nal_ref_idc				: 2;
		BYTE forbidden_zero_bit			: 1;
	};

	//FU-Aͷ��FU-A��Ƭ����ķ�Ƭͷ��
	struct  FU_A_header
	{
		byte type		: 5;			//nal����
		byte R			: 1;			//����Ϊ0
		byte E			: 1;			//�������λ
		byte S			: 1;			//��ʼ���λ
	};

	//FU-Aͷ��FU-A��Ƭ����ķ�Ƭͷ��
	struct  FU_A_h265_header
	{
		byte type : 6;			//nal����
		//byte R : 1;			//����Ϊ0
		byte E : 1;			//�������λ
		byte S : 1;			//��ʼ���λ
	};

	//hevc
	typedef struct {
		uint8_t forbidden_zero_bit; // 1 bit
		uint8_t nal_unit_type;      // 6 bits
		uint8_t nuh_layer_id;       // 6 bits
		uint8_t nuh_temporal_id_plus1; // 3 bits
	} NALHeader_hevc;
#pragma pack()

	//�洢sps��pps NAL��Ԫ�Ļ�����
	Tool::TBuff<BYTE> m_buffSPS;
	Tool::TBuff<BYTE> m_buffPPS;

	//VPS h265 ���ӵĲ�����
	Tool::TBuff<BYTE> m_buffVPS;
	//sps��pps��NAL��Ԫ���������ʱ���
	//CCriticalSection m_lockSPPS;
	std::mutex		   m_lockSPPS;

	//�洢һ��Rtp��
	Tool::TBuff<BYTE> m_buffRtp;

	//�洢һ��Nalu��Ԫ
	Tool::TBuff<BYTE> m_buffNalu;

	//Rtp��ͷ
	RtpHeader m_RtpHeader;

	//RTP����ص���������������
	CT_Rtp m_pfnRtp;
	LPVOID m_lpRtp;

protected:

	//h264 ��Nalu�����RTP��
	void Nalu(UINT uNextDTS);

	//h265����������Nalu�����RTP��
	void Nalu_hevc(UINT uNextDTS);

	BOOL IsIFrameStart(BYTE *pBuff, UINT nLen);

	//�ӻ����������޷��Ÿ��ײ�����ֵ
	UINT GetUeValue(BYTE *pBuff, UINT nLen, UINT &nStartBit);

	//���Rtpͷ��Ϣ
	void ResetRtpHeader(RtpHeader &header);

};

}
