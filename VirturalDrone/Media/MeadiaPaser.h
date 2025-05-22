#pragma once
#include <thread>
#include <string>
#include <mutex>
#include <list>
#include <condition_variable>
#include "Tool/XEvent.h"
#include "Media/Es2Rtp.h"
#include "mqtt/CProduct.h"
#include "windows/Monitor.h"

#include "ps_demux.h"
#include "ps_struct.h"

struct AVFormatContext;
struct AVCodecContext;
struct AVDictionary;
struct AVCodec;
struct AVFrame;
struct AVPacket;
struct SwsContext;
struct AVBufferRef;
struct AVBSFContext;
struct AVBitStreamFilter;
struct AVCodecParameters;

#ifdef _WIN32
//��׼��Tick
static DWORD Tick()
{
	LARGE_INTEGER count, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return (DWORD)(count.QuadPart * 1000 / freq.QuadPart);
}
#else
//#define unsigned long DWORD
//��׼��Tick
static unsigned long Tick()
{
	struct timeval tv = { 0 };
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec;
}

#endif // _WIN32

typedef void(*CT_RtpRecv)(void* lpContext,	//��������
	unsigned char* pData,						//����֪ͨ�Ŀͻ���ָ��
	unsigned int nSize
	);

struct TimeWait
{
	bool bFirst;

	DWORD dwStartDecodeStamp;
	DWORD dwStartTickStamp;
	int nDelayTime;
	xbase::XEvent wait_event;
	//����
	TimeWait()
		: bFirst(true)
		, dwStartDecodeStamp(-1)
		, dwStartTickStamp(-1)
		, nDelayTime(0)
	{}

	void Clear()
	{
		bFirst = true;
	}

	long long CalDiff(DWORD dwTime)
	{
		if (bFirst)
		{
			dwStartDecodeStamp = dwTime;
			dwStartTickStamp = Tick() + nDelayTime;
			bFirst = false;
			return 0;
		}
		else {
			long long dwDiffDecode = dwTime - dwStartDecodeStamp;
			long long dwDiffTick = Tick() - (dwStartTickStamp + nDelayTime);

			return dwDiffDecode - dwDiffTick;
		}
	}

	//�����ӳ�ʱ��
	/*nDiff�� �ӳ�ʱ�����*/
	void SetDelay(int nDiff) //
	{
		nDelayTime = nDiff;
	}

	//����ȴ�ʱ��
	long long CalWait(DWORD dwTime)
	{
		int ret = CalDiff(dwTime);
		if (abs(ret) > 10000)
		{
			Clear();
			return CalDiff(dwTime);
		}

		return ret;
	}

	//����ʱ����ȴ�
	bool Wait(DWORD dwTimeStamp)
	{
		long long  dwWait = CalWait(dwTimeStamp);

		if (dwWait <= 0)
		{
			return true;
		}
		wait_event.TryWait(dwWait);
		return true;
	}
};

class CMeadiaPaser
{
public:
	CMeadiaPaser();
	~CMeadiaPaser();

	int Start(std::string strUrl);
	int Stop();

	bool AddClient(std::string strCallID, CT_RtpRecv pFn, void* pContext);
	bool RemoveClient(std::string strCallID);

	void StreamReciveThread();
	void Es2RtpThread();

	bool InitMqtt(std::string strID);
	bool DeInitMqtt();

	bool StartMqtt();
	bool EndMqtt();
protected:
	int InitFFmpeg();
	void DeInitFFmpeg();
	void _input_video_queue(const AVPacket& pkt);
	void _clear_packet_video_queue();
	std::string GetFFmepgStrError(int errcode);
	bool _is_file_player();

	static void RtpCB_s(
		LPVOID lpContext,
		byte* pRtp,
		int nLen,
		int nPayloadLen,
		UINT nDTS
	);

	void RtpCB(
		byte* pRtp,
		int nLen,
		int nPayloadLen,
		UINT nDTS
	);

	//PS ��װH265/H264 ,���װES�ص�
	static void es_callback_s(unsigned char* es_data, int es_data_len, PS_ESParam_S es_param, void* user_param);
	void es_callback_(unsigned char* es_data, int es_data_len, PS_ESParam_S es_param);

	//�ֶ�����codecpar
	bool CreateCodecpar();

	//������̽���������/H264 or H265
	bool probe_stream_type(const AVPacket* pkt, std::string& strType);
	bool extract_sps_pps_from_packet(AVCodecParameters* codecpar, const AVPacket* pkt);			//����h264 sps
	bool extract_sps_pps_from_packet_hevc(AVCodecParameters* codecpar, const AVPacket* pkt);	//����hevc sps
protected:
	std::string m_strUrl;
	bool		m_bStart;
	bool		m_bStartStreamThread;
	bool		m_bStartDispatchThread;

	std::thread m_ThreadStreamRecive;
	std::thread m_ThreadDispatch;

	std::mutex			m_packet_mutex;
	std::list<AVPacket> m_packet_queue;

	TimeWait			m_timewait;

	//ffmpeg ���
	AVDictionary*		m_ffmpeg_options;
	AVFormatContext*	m_AVFormatContext;
	std::mutex			m_mutexInitFFmpeg;
	int					m_nVideoStreamIndex;
	const AVCodec*		m_AVCodec;
	AVCodecContext*		m_AVCodecContext;
	std::condition_variable	m_condition;
	AVBSFContext*		m_bsf_ctx;
	const AVBitStreamFilter*  m_bsfilter;


	Media::CEs2Rtp		m_es2Rtp;

	std::map<std::string, std::pair<CT_RtpRecv, void*>> m_mapRevClient2; // ֱ�ӻص�sip �ͻ���
	std::mutex		   m_mutexMapRecvClient2;

	//mqtt
	CProduct		   m_product;
	//Config::ChNode	   m_chNode;

	bool			   m_bInitFFmpeg;

	bool			   m_bFileStream;
	std::shared_ptr<CMonitor> m_pMonitor;

	bool bFindPPS = false;
	bool bFindSPS = false;
	bool bNeedDemuxPS = false;
	CParsePS m_psParse;
	std::list<AVPacket> m_packet_queue_temp; //��Ŵ���codecparʱ��ȡ��paket
};

