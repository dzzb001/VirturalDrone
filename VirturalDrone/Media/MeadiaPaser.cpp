#include "stdafx.h"
#include "MeadiaPaser.h"
#include "Tool/XTime.h"
#include "Es2Rtp.h"
#include "Http.h"
#include "Tool/cJSON.h"
#include "MonitorManager.h"

#include "h26x_sps_parse/h26x_sps_parse.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
#include "libavutil/hwcontext.h" 
#include <libavcodec/bsf.h>
}

#define Log LogN(111)
#define L_QUERY_PLAY_URL				"/admin-api/ndnp/openApi/getDahuaRtsp"

#define DAHUA_DSS 1
#define FILTER  1
using namespace std;

static const  char* g_crypto_type = "file,crypto,data";
static const int64_t g_crypto_size = strlen(g_crypto_type);

int testBitStreamFlilt()
{
    const char* default_filename = "F:\\drone\\1.mp4";
    FILE* outfp = fopen("mp4ToH264.h264", "wb");
    //1:分配接复用器上下文
    AVFormatContext* ctx = NULL;

    int vedio_index = -1;
    //2:打开文件
    int ret = avformat_open_input(&ctx, default_filename, NULL, NULL);
    if (ret != 0)
    {
        printf("[error]avformat_open_input\n");
        if (ctx)
        {
            avformat_close_input(&ctx);
        }
        getchar();
    }
    //3:获取信息
    ret = avformat_find_stream_info(ctx, NULL);
    if (ret < 0)
    {
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf));
        printf("open %s failed:%s\n", default_filename, buf);
        if (ctx)
        {
            avformat_close_input(&ctx);
        }
        getchar();
    }
    //4:找到视频流
    vedio_index = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (vedio_index < 0)
    {
        printf("av_find_best_Stream failed");
        return 0;
    }
    //5:读取一帧数据
    AVPacket* pkt = NULL;
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    //ffmpeg -bsfs命令可以查看ffmpeg工具支持的Bitstream Filter类型。
    const AVBitStreamFilter* bsfilter = av_bsf_get_by_name("h264_mp4toannexb");

    //6:初始化过滤器上下文
    AVBSFContext* bsf_ctx = NULL;
    av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;

    //7：添加解码器属性，从AVFormatContext中获取
    avcodec_parameters_copy(bsf_ctx->par_in, ctx->streams[vedio_index]->codecpar);
    if (av_bsf_init(bsf_ctx) < 0) {
        OutputDebugString("av_bsf_init is error\n");
        return -1;
    }

    //8：读取一帧数据
    while (av_read_frame(ctx, pkt) >= 0)
    {
        if (pkt->stream_index == vedio_index)
        {
            int input_size = pkt->size;
            int out_pkt_count = 0;
            if (av_bsf_send_packet(bsf_ctx, pkt) != 0)
            {
                av_packet_unref(pkt);
                continue;
            }
            av_packet_unref(pkt);   // 释放资源
            while (av_bsf_receive_packet(bsf_ctx, pkt) == 0)
            {
                out_pkt_count++;
                size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
                if (size != pkt->size)
                {
                    printf("fwrite failed-> write:%u, pkt_size:%u\n", size, pkt->size);
                }
                av_packet_unref(pkt);
            }
            // TS流可以直接写入
            /*size_t size = fwrite(pkt->data, 1, pkt->size, outfp);
            if(size != pkt->size)
            {
                printf("fwrite failed-> write:%u, pkt_size:%u\n", size, pkt->size);
            }*/
            av_packet_unref(pkt);
        }
        else
        {
            if (ret == 0)
                av_packet_unref(pkt);        // 释放内存
        }
    }
    if (outfp)
        fclose(outfp);
    printf("success");
}

static int interruptCallback(void* context) 
{
    CMeadiaPaser* pThis = (CMeadiaPaser*)context;
    if (pThis == NULL) {
        return 0;
    }
    return 0;
}
static void ff_log_callback(void* ptr, int level, const char* fmt, va_list vargs)
{
	char buf[1024] = { 0 };
	vsnprintf(buf, 1023, fmt, vargs);
	OutputDebugString(buf);
}

CMeadiaPaser::CMeadiaPaser() : m_bStart(false)
, m_ffmpeg_options(nullptr)
, m_AVFormatContext(nullptr)
, m_AVCodecContext(nullptr)
, m_bsf_ctx(nullptr)
, m_bsfilter(nullptr)
, m_bInitFFmpeg(false)
, m_bFileStream(true)
{
	avformat_network_init();
#ifndef DEBUG
//	av_log_set_level(AV_LOG_DEBUG);
//	av_log_set_callback(ff_log_callback);
#else
	av_log_set_level(AV_LOG_QUIET);
#endif
}
CMeadiaPaser::~CMeadiaPaser()
{
    m_condition.notify_all();
}
int CMeadiaPaser::Start(std::string strUrl)
{
	if (m_bStart)
		return 0;
    m_bStart = true;
    m_es2Rtp.RegRtp(RtpCB_s, this);

    if (strUrl.find("DSS_DEV_ID") == 0)
    {
        //从平台获取url
        std::shared_ptr<CHttp> pHttp = CContextBase::Create<CHttp>();

        std::string strId;
        int nPos = strUrl.find("DSS_DEV_ID=");
        strId = strUrl.substr(strlen("DSS_DEV_ID=") + nPos);

        std::string ip = Config::CConfig::GetInstance().GetDaHuaSerIP();
        int nPort = Config::CConfig::GetInstance().GetDaHuaSerPort();
        m_pMonitor = Tool::CMonitorManager::GetInstance().GetMonitor();

        if (pHttp->Connect(m_pMonitor, ip, nPort, L_QUERY_PLAY_URL)) {
            Log(Tool::Error, "%s connect agent failed.[%s][%d][%x]", __FUNCTION__, ip.c_str(), nPort, m_pMonitor); 
            pHttp->DisConnect();
            return false;
        }

        std::vector<std::pair<std::string, std::string>> vecHead;
        std::vector<std::pair<std::string, std::string>> vecParams;
        vecHead.push_back(std::make_pair<std::string, std::string>("Content-Type", "application/json;charset=UTF-8"));
        vecParams.push_back(std::make_pair<std::string, std::string>("id", strId.c_str()));

        if (!pHttp->Get(vecParams, vecHead))
        {
            cJSON* pJson = cJSON_Parse(pHttp->m_httpRespone.m_strContent.c_str());
            if (pJson)
            {
                std::string strMsg;
                int code = 0;
                cJSON* pItem = cJSON_GetObjectItem(pJson, "code");
                if (pItem)
                {
                    code = pItem->valueint;
                }
                               
                pItem = cJSON_GetObjectItem(pJson, "msg");
                if (pItem)
                {
                    if (pItem->valuestring)
                    {
                        strMsg = pItem->valuestring;
                    }
                }

                if (code)
                {
                    Log(Tool::Error, "L_QUERY_PLAY_URL code:%d, dscrp:%s", code, strMsg.c_str());
                    return false;
                }

                pItem = cJSON_GetObjectItem(pJson, "data");
                if (pItem)
                {
                    if (pItem->valuestring)
                    {
                        m_strUrl = pItem->valuestring;
                    }
                }
            }
        }///
        else {
            Log(Tool::Debug, "connect dahua proxy server failed[%s][%d]", ip.c_str(), nPort);
        }
        pHttp->DisConnect();
    }else
	    m_strUrl = strUrl;

    Log(Tool::Debug, "url=%s", m_strUrl.c_str());

    m_bStartStreamThread = true;
	m_ThreadStreamRecive = std::thread(&CMeadiaPaser::StreamReciveThread, this);
    m_bStartDispatchThread = true;
	m_ThreadDispatch = std::thread(&CMeadiaPaser::Es2RtpThread, this);

    m_product.NotifyStartSendData();
	return 0;
}
int CMeadiaPaser::Stop()
{
    if (!m_bStart)
        return 0;

    if (m_bStartDispatchThread) {
        m_bStartDispatchThread = false;
        m_condition.notify_all();
        m_ThreadDispatch.join();
    }

    if (m_bStartStreamThread)
    {
        m_bStartStreamThread = false;
        m_ThreadStreamRecive.join();
    }

    DeInitFFmpeg();
    m_bStart = false;
	return 0;
}

std::string CMeadiaPaser::GetFFmepgStrError(int errcode)
{
    char errName[1024] = { 0 };
    if (av_strerror(errcode, errName, 1024) == 0)
        return errName;

    return std::to_string(errcode);
}

int CMeadiaPaser::InitFFmpeg()
{
    int ret = 0;

    //av_dict_set(&options, "rtsp_transport", "udp", 0); // 设置udp播放录像时长时间pause 会导致无法恢复的问题
    // 设置传输协议为TCP协议
    av_dict_set(&m_ffmpeg_options, "rtsp_transport", "tcp", 0);
    //
    // 设置TCP连接最大延时时间
    av_dict_set(&m_ffmpeg_options, "max_delay", "100", 0);

    // 设置“buffer_size”缓存容量
    av_dict_set(&m_ffmpeg_options, "buffer_size", "1024000", 0);

    if (!m_AVFormatContext)
    {
        this->m_AVFormatContext = avformat_alloc_context();
        if (!m_AVFormatContext)
        {
            Log(Tool::Error, "avformat alloc context ptr failed !!!");
            return -1;
        }
        //m_AVFormatContext->flags |= AVFMT_FLAG_NONBLOCK; //设置成非阻塞
        m_AVFormatContext->interrupt_callback.opaque = (void*)this;
        m_AVFormatContext->interrupt_callback.callback = interruptCallback;//设置回调

        ret = avformat_open_input(&m_AVFormatContext, m_strUrl.c_str(), NULL, &m_ffmpeg_options);
        if (ret < 0)
        {
            Log(Tool::Warning, "[%s][%d][%d][url = %s]FFDecoder open input failed, err: %s", __FUNCTION__, __LINE__, ::time(NULL),m_strUrl.c_str(), GetFFmepgStrError(ret).c_str());
            return -1;
        }

        for (unsigned i = 0; i < m_AVFormatContext->nb_streams; i++)
        {
            if (m_AVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                m_nVideoStreamIndex = i;
                break;
            }
        }
        if (m_nVideoStreamIndex == -1) {
            Log(Tool::Debug, "FFDecoder could find video stream, url = %s", m_strUrl.c_str());
            return -1;
        }

        AVCodecParameters* codecpar = m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar;
        if (codecpar->codec_id != AV_CODEC_ID_NONE)//未识别到AV_CODEC_ID，不需要调用此接口，否则此接口会一直无意义探测大概直到7、8停止，。导致卡顿延迟严重
        {
            ret = avformat_find_stream_info(m_AVFormatContext, nullptr);
        }

        if (ret < 0)
        {
            Log(Tool::Warning, "[%s][%d][%d][url = %s]Couldn't find stream information.", __FUNCTION__, __LINE__, ::time(NULL), m_strUrl.c_str());
            return -1;
        }
    }


    //初始化PS解析器
    m_psParse.set_es_callback(es_callback_s, this);
    m_psParse.init_parse();

    AVCodecParameters* codecpar = m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar;
    if (!codecpar->extradata || codecpar->extradata_size <= 0)
    {
        if (!CreateCodecpar())
            return false;
    }

    m_AVCodec = nullptr;
    if (!m_AVCodec)
    {
        if (NULL == m_AVCodec)
        {
            m_AVCodec = (AVCodec*)avcodec_find_decoder(m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->codec_id);
            if (NULL == m_AVCodec)
            {
                Log(Tool::Error, "FFDecoder find AVCodec failed, url = %s, codec: %s", m_strUrl.c_str(), avcodec_get_name(m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->codec_id));
                return -1;
            }
        }
    }

    if (!m_AVCodecContext)
    {
        m_AVCodecContext = avcodec_alloc_context3(m_AVCodec);
        if (m_AVCodecContext == NULL) {
            Log(Tool::Error,"Could not allocate AVCodecContext");
            return -1;
        }
        ret = avcodec_parameters_to_context(m_AVCodecContext, m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar);
        if (ret < 0)
        {
            Log(Tool::Error, "FFDecoder avcodec_parameters_to_context failed, url = %s, err = %s", m_strUrl.c_str(), GetFFmepgStrError(ret).c_str());
            return -1;
        }

         ret = avcodec_open2(m_AVCodecContext, m_AVCodec, NULL);
         if (ret < 0)
         {
             Log(Tool::Error, "FFDecoder AVCodec Open  failed, url = %s, codec: %s, err = %s", m_strUrl.c_str(), avcodec_get_name(m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->codec_id), GetFFmepgStrError(ret).c_str());
             return -1;
         }
    }
    
    //ffmpeg -bsfs命令可以查看ffmpeg工具支持的Bitstream Filter类型。
#if FILTER
    if (m_AVCodec->id == AV_CODEC_ID_HEVC) {
        m_bsfilter = av_bsf_get_by_name(/*"hevc_metadata"*/"hevc_mp4toannexb");
        m_es2Rtp.SetEncodeType(Media::CEs2Rtp::eEncodeType::H265);
    }
    else {
        m_bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
        m_es2Rtp.SetEncodeType(Media::CEs2Rtp::eEncodeType::H264);
    }

    if (!m_bsfilter)
    {
        Log(Tool::Error, "bsf alloc failed");
        return -1;
    }

    //申请过滤器
    ret = av_bsf_alloc(m_bsfilter, &m_bsf_ctx); //AVBSFContext;
    if (ret)
    {
        Log(Tool::Error, "FFDecoder av_bsf_alloc failed,%s",GetFFmepgStrError(ret).c_str());
    }

    //添加解码器属性，从AVFormatContext中获取
    ret = avcodec_parameters_copy(m_bsf_ctx->par_in, m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar);
    if (ret<0)
    {
        Log(Tool::Error, "FFDecoder avcodec_parameters_copy failed,%s", GetFFmepgStrError(ret).c_str());
        return -1;
    }
    
    //初始化过滤器上下文
    if (av_bsf_init(m_bsf_ctx) < 0) {
        Log(Tool::Error, "av_bsf_init is error");
        return -1;
    }
#endif
    m_bFileStream = _is_file_player();
    return 0;
}

void CMeadiaPaser::DeInitFFmpeg()
{
    if (m_ffmpeg_options)
    {
        av_dict_free(&m_ffmpeg_options);
        m_ffmpeg_options = nullptr;
    }

    if (m_AVCodecContext)
    {
        avcodec_close(m_AVCodecContext);
        avcodec_free_context(&m_AVCodecContext);
        m_AVCodecContext = nullptr;
    }

    m_AVCodec = nullptr;
    if (this->m_AVFormatContext)
    {
        avformat_flush(m_AVFormatContext);
        avformat_close_input(&m_AVFormatContext);
        avformat_free_context(m_AVFormatContext);
        m_AVFormatContext = NULL;
    }
    if (m_bsf_ctx)
    {
        av_bsf_free(&m_bsf_ctx);
    }
}
bool CMeadiaPaser::AddClient(std::string strCallID, CT_RtpRecv pFn, void* pContext)
{
    bool ret = true;
    std::lock_guard<std::mutex> lock(m_mutexMapRecvClient2);
    std::map<std::string, std::pair<CT_RtpRecv, void*>>::iterator itor = m_mapRevClient2.find(strCallID);
    if (itor != m_mapRevClient2.end())
        ret = false;
    else {
        m_mapRevClient2[strCallID] = std::make_pair(pFn, pContext);
    }
    return ret;
}
bool CMeadiaPaser::RemoveClient(std::string strCallID)
{
    bool ret = true;
    std::lock_guard<std::mutex> lock(m_mutexMapRecvClient2);
    std::map<std::string, std::pair<CT_RtpRecv, void*>>::iterator itor = m_mapRevClient2.find(strCallID);
    if (itor != m_mapRevClient2.end()) {
        m_mapRevClient2.erase(itor);
    }
    return ret;
}
void CMeadiaPaser::_input_video_queue(const AVPacket& pkt)
{
    {
        std::lock_guard<std::mutex>  lock(m_packet_mutex);
        m_packet_queue.push_back(pkt);
    }
    m_condition.notify_one();
}

void CMeadiaPaser::_clear_packet_video_queue()
{
    std::lock_guard < std::mutex> lock{ m_packet_mutex };
    AVPacket pkt;
    while (!m_packet_queue.empty())
    {
        pkt = m_packet_queue.front();
        m_packet_queue.pop_front();
        av_packet_unref(&pkt);
    }

}
void CMeadiaPaser::StreamReciveThread()
{
    bool isEof = false;
    int ret = 0;
    int nCount = 10;

    bool bEndThread = false;
    while (true)
    {
        if (!m_bInitFFmpeg) {
            while (m_bStartStreamThread && !bEndThread)
            {
                if (InitFFmpeg())
                {
                    Log(Tool::Debug, "InitFFmpeg error\n");
                    DeInitFFmpeg();
                    m_bInitFFmpeg = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    nCount--;

                    if (nCount < 0) {
                        bEndThread = true;
                    }
                    continue;
                }
                else
                {
                    m_bInitFFmpeg = true;
                    m_product.NotifyStartSendData();
                    break;
                }
            }
        }

        if (!m_bStartStreamThread || bEndThread)
        {
            _clear_packet_video_queue();
            DeInitFFmpeg();
            m_bInitFFmpeg = false;
            break;
        }
        if (m_packet_queue.size() > 20)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        AVPacket packet;
        ret = av_read_frame(m_AVFormatContext, &packet);
        if (ret < 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::string szErrName = GetFFmepgStrError(ret).c_str();
            Log(Tool::Warning, "[%s][%d][%d][url = %s][szErrName = %s]", __FUNCTION__, __LINE__, ::time(NULL), m_strUrl,szErrName.c_str());
            if (szErrName != "End of file")
            {
                Log(Tool::Debug, " av_read_frame error %s\n", szErrName.c_str());
                //报错了重新播放
                _clear_packet_video_queue();
                DeInitFFmpeg();
                m_bInitFFmpeg = false;
            }
            else
            {
                isEof = true;
                Log(Tool::Debug, " av_read_frame End of file");
               
                // 
                //step2 重新从头开始播放文件
                _clear_packet_video_queue();
                DeInitFFmpeg();
                m_bInitFFmpeg = false;
            }
        }


        //add at 20250421
        if (packet.stream_index == m_nVideoStreamIndex && bNeedDemuxPS)
        {
            AVPacket pkt;
            while (!m_packet_queue_temp.empty())
            {
                pkt = m_packet_queue_temp.front();
                m_packet_queue_temp.pop_front();

                m_psParse.put_pkt_data(pkt.data, pkt.size);
                av_packet_unref(&pkt);
            }
            m_psParse.put_pkt_data(packet.data, packet.size);
            av_packet_unref(&packet);
            continue;
        }
        //add end

        if (packet.stream_index == m_nVideoStreamIndex && packet.dts != AV_NOPTS_VALUE)
        {
            _input_video_queue(packet);
        }
        else
        {
            av_packet_unref(&packet);
        }
    }

    m_condition.notify_all();
    _clear_packet_video_queue();
    Log(Tool::Info,"%s read frame thread exit!", m_strUrl.c_str());
}

void CMeadiaPaser::Es2RtpThread()
{
    while (true)
    {
        if (!m_bStartDispatchThread)
            break;

        if (m_packet_queue.empty() && m_bStartDispatchThread)
        {
            std::unique_lock<std::mutex> lock{ m_packet_mutex };
            m_condition.wait(lock, [this]() {return !m_packet_queue.empty() || m_bStartDispatchThread == false;  });

          //m_condition.wait(lock, [this]() {return !m_packet_queue.empty() || m_decoder_thread_status != datautil::Decoder_Thread_Run;  });
        }

        {
            std::lock_guard <std::mutex> lock{ m_packet_mutex };
            if (m_packet_queue.empty())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            AVPacket packet = m_packet_queue.front();
            AVPacket* pkt = &packet;

            //AVPacket 数据形式有两种
            //1、对于由原始数据编码得到的AVPacket格式与标准格式相同，即起始码startcode+实际数据
            //2、对于解封装得到的AVPacket格式为四字节长度+实际数据 用前四个字节表示实际数据长度 
            // 我们将 第二种格式转换成第一种格式。
#if FILTER
            {
                int input_size = pkt->size;
                int out_pkt_count = 0;
                if (av_bsf_send_packet(m_bsf_ctx, pkt) != 0)
                {
                    av_packet_unref(pkt);
                    m_packet_queue.pop_front();
                    continue;
                }

                av_packet_unref(pkt);   // 释放资源
                m_packet_queue.pop_front();

                while (av_bsf_receive_packet(m_bsf_ctx, pkt) == 0)
                {
                    UINT uTimestamp = pkt->dts * 1000 / m_AVFormatContext->streams[m_nVideoStreamIndex]->time_base.den;
                    out_pkt_count++;

                    UINT uPts = uTimestamp * 90000 / 1000;  //RTP流时间戳按照90K采样率
                    m_es2Rtp.EsIn(pkt->data, pkt->size, uPts);

                    if(m_bFileStream)
                        m_timewait.Wait(uTimestamp);
                    av_packet_unref(pkt);
                }
            }
#else
            UINT uTimestamp = pkt->dts * 1000 / m_AVFormatContext->streams[m_nVideoStreamIndex]->time_base.den;
            //out_pkt_count++;

            UINT uPts = uTimestamp * 90000 / 1000;  //RTP流时间戳按照90K采样率
            m_es2Rtp.EsIn(pkt->data, pkt->size, uPts);

            char buf[512] = { 0 };
            sprintf(buf, "pts %d", uPts);
            OutputDebugString(buf);
            av_packet_unref(pkt);
            m_packet_queue.pop_front();
#endif
        }
    }

    Log(Tool::Debug, "rtp thread exit!\n");
}

//ES2Rtp的ES输出回调函数
void CMeadiaPaser::RtpCB_s(LPVOID lpContext, byte* pRtp, int nLen, int nPayloadLen, UINT nDTS)
{
    CMeadiaPaser* pThis = (CMeadiaPaser*)lpContext;

    /*char buf[512] = { 0 };
    sprintf(buf, "**************** data %x, len %d nDTS %d nPayloadLen %d\n", pRtp, nLen, nDTS / 90, nPayloadLen);
    OutputDebugString(buf);*/

    if (pThis)
    {
        pThis->RtpCB(pRtp, nLen, nPayloadLen, nDTS);
    }
}

void CMeadiaPaser::RtpCB(
    byte* pRtp,
    int nLen,
    int nPayloadLen,
    UINT nDTS
)
{
    std::lock_guard<std::mutex> lock(m_mutexMapRecvClient2);
    std::map<std::string, std::pair<CT_RtpRecv, void*>>::iterator itor = m_mapRevClient2.begin();
    for (; itor != m_mapRevClient2.end(); itor++) {
        if (itor->second.first)
        {
            itor->second.first(itor->second.second, (unsigned char*)pRtp, nLen);
        }
    }
}

bool CMeadiaPaser::InitMqtt(std::string strID)
{
    m_product.InitMqtt(strID);
    return true;
}

bool CMeadiaPaser::DeInitMqtt()
{
    m_product.DeInitMqtt();
    return true;
}

bool CMeadiaPaser::StartMqtt()
{
    return m_product.Start();
}
bool CMeadiaPaser::EndMqtt()
{
    return m_product.End();
}


bool CMeadiaPaser::_is_file_player()
{
    if (!m_AVFormatContext || !m_AVFormatContext->protocol_whitelist)
    {
        return false;
    }
    int size = strlen(m_AVFormatContext->protocol_whitelist);
    if (size == g_crypto_size && memcmp(m_AVFormatContext->protocol_whitelist, g_crypto_type, g_crypto_size) == 0)
    {
        return true;
    }
    return false;
}


void CMeadiaPaser::es_callback_s(unsigned char* es_data, int es_data_len, PS_ESParam_S es_param, void* user_param)
{
    if (user_param)
    {
        CMeadiaPaser* pThis = (CMeadiaPaser*)user_param;
        if (pThis) {
            pThis->es_callback_(es_data, es_data_len, es_param);
        }
    }
}

void CMeadiaPaser::es_callback_(unsigned char* es_data, int es_data_len, PS_ESParam_S es_param)
{
    switch (es_param.video_param.frame_type)
    {
    case PS_ES_FRAME_TYPE_DATA:
        break;
    case PS_ES_FRAME_TYPE_IDR:
        break;
    case PS_ES_FRAME_TYPE_SEI:
        break;
    case PS_ES_FRAME_TYPE_SPS:
        break;
    case PS_ES_FRAME_TYPE_PPS:
        break;
    default:
        break;
    }

    AVPacket full_pkt;
    av_new_packet(&full_pkt, es_data_len);
    memcpy(full_pkt.data, es_data, es_data_len);

    full_pkt.pts = es_param.video_param.pts;
    full_pkt.dts = es_param.video_param.dts;
    if (full_pkt.dts == 0)
        full_pkt.dts = full_pkt.pts;

    if (es_param.video_param.is_i_frame)
        full_pkt.flags |= AV_PKT_FLAG_KEY;
    else
        full_pkt.flags = 0;

    full_pkt.size = es_data_len;

    _input_video_queue(full_pkt);
}

bool CMeadiaPaser::CreateCodecpar()
{
    //构造extradata
    AVPacket pkt;
    int ret = -1;
    while (1) {
        ret = av_read_frame(m_AVFormatContext, &pkt);
        if (ret < 0)
            break;

        if (pkt.stream_index == m_nVideoStreamIndex) {
            //探测编码类型
            std::string strType;
            bool ret = probe_stream_type(&pkt, strType);
            if (!ret)
            {
                av_packet_unref(&pkt);
                continue;
            }

            if (!strType.compare("H264")) {
                // 解析NAL单元
                if (extract_sps_pps_from_packet(m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar, &pkt)) {
                    //av_packet_unref(&pkt);
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->codec_id = AV_CODEC_ID_H264;
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->format = 12;
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->field_order = AV_FIELD_PROGRESSIVE;

                    m_packet_queue_temp.push_back(pkt);
                    m_psParse.set_hevc_ps(false);
                    break;
                }
            }
            else if (!strType.compare("HEVC")) {
                // 解析NAL单元
                if (extract_sps_pps_from_packet_hevc(m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar, &pkt)) {
                    //av_packet_unref(&pkt);
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->codec_id = AV_CODEC_ID_HEVC;
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->format = 12;
                    m_AVFormatContext->streams[m_nVideoStreamIndex]->codecpar->field_order = AV_FIELD_PROGRESSIVE;
                    m_packet_queue_temp.push_back(pkt);

                    m_psParse.set_hevc_ps(true);
                    break;
                }
            }

            av_packet_unref(&pkt);
        }
    }
    return true;
}

//判断这个packet的编码类型/H264,H265
bool CMeadiaPaser::probe_stream_type(const AVPacket* pkt, std::string& strType)
{
    const uint8_t* data = pkt->data;
    int size = pkt->size;
    int offset = 0;

    while (offset < size - 3) {
        // 查找起始码 (00 00 01 或 00 00 00 01)
        if ((data[offset] == 0x00 && data[offset + 1] == 0x00 && data[offset + 2] == 0x01) ||
            (offset < size - 4 && data[offset] == 0x00 && data[offset + 1] == 0x00 &&
                data[offset + 2] == 0x00 && data[offset + 3] == 0x01)) {

            int start_code_len = (data[offset + 2] == 0x01) ? 3 : 4;

            if (data[offset + start_code_len] == 0xba) //ps head
            {
                bNeedDemuxPS = true;
            }

            int nal_start = offset + start_code_len;
            if (nal_start >= size) break;

            uint8_t nal_type = 0;
            uint8_t nal_type_hevc = 0;

            nal_type = data[nal_start] & 0x1F;  //0x01 0xba  0x01 0xbb 0x01 0xbc
            nal_type_hevc = (data[nal_start] >> 1) & 0x3F; //hevc nal head 2个字节 ，取 NAL 头 第一个字节的高 6 位

            // 查找下一个起始码
            int next_start = nal_start;
            while (next_start < size - 3) {
                if ((data[next_start] == 0x00 && data[next_start + 1] == 0x00 && data[next_start + 2] == 0x01) ||
                    (next_start < size - 4 && data[next_start] == 0x00 && data[next_start + 1] == 0x00 &&
                        data[next_start + 2] == 0x00 && data[next_start + 3] == 0x01)) {
                    break;
                }
                next_start++;
            }

            int nal_size = (next_start < size) ? (next_start - nal_start) : (size - nal_start);

            if (nal_type == 7 || nal_type == 8) {
                strType = "H264";
                return true;
            }

            if (nal_type_hevc == 32 || nal_type_hevc == 33 || nal_type_hevc == 34) { //VPS/SPS/PPS
                strType = "HEVC";
                return true;
            }

            offset = next_start;
        }
        else {
            offset++;
        }
    }
    return false;
}

bool CMeadiaPaser::extract_sps_pps_from_packet(AVCodecParameters* codecpar, const AVPacket* pkt) {
    const uint8_t* data = pkt->data;
    int size = pkt->size;
    int offset = 0;

    while (offset < size - 3) {
        // 查找起始码 (00 00 01 或 00 00 00 01)
        if ((data[offset] == 0x00 && data[offset + 1] == 0x00 && data[offset + 2] == 0x01) ||
            (offset < size - 4 && data[offset] == 0x00 && data[offset + 1] == 0x00 &&
                data[offset + 2] == 0x00 && data[offset + 3] == 0x01)) {

            int start_code_len = (data[offset + 2] == 0x01) ? 3 : 4;

            if (data[offset + start_code_len] == 0xba) //ps head
            {
                bNeedDemuxPS = true;
            }

            int nal_start = offset + start_code_len;
            if (nal_start >= size) break;

            uint8_t nal_type = 0;

            nal_type = data[nal_start] & 0x1F;  //0x01 0xba  0x01 0xbb 0x01 0xbc

            // 查找下一个起始码
            int next_start = nal_start;
            while (next_start < size - 3) {
                if ((data[next_start] == 0x00 && data[next_start + 1] == 0x00 && data[next_start + 2] == 0x01) ||
                    (next_start < size - 4 && data[next_start] == 0x00 && data[next_start + 1] == 0x00 &&
                        data[next_start + 2] == 0x00 && data[next_start + 3] == 0x01)) {
                    break;
                }
                next_start++;
            }

            int nal_size = (next_start < size) ? (next_start - nal_start) : (size - nal_start);

            if (nal_type == 7 || nal_type == 8 || nal_type == 5) {
                // 跳过已处理的 SPS/PPS
                if ((nal_type == 7 && bFindSPS) || (nal_type == 8 && bFindPPS)) {
                    offset = next_start;
                    continue;
                }

                // 计算新数据大小（不含填充字节）
                int new_data_size = codecpar->extradata_size + 4 + nal_size;
                uint8_t* new_data = (uint8_t*)av_realloc(codecpar->extradata, new_data_size + AV_INPUT_BUFFER_PADDING_SIZE);
                if (!new_data) return false;

                codecpar->extradata = new_data;
                int write_pos = codecpar->extradata_size;

                // 写入起始码和 NAL 数据
                uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
                memcpy(codecpar->extradata + write_pos, start_code, 4);
                memcpy(codecpar->extradata + write_pos + 4, data + nal_start, nal_size);
                codecpar->extradata_size = new_data_size;

                // 填充末尾
                memset(codecpar->extradata + write_pos + 4 + nal_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

                // 解析 SPS
                if (nal_type == 7) {
                    monk::H26xSPSInfo h264_info;
                    int32_t result = monk::H26xParseSPS::h264Parse((const uint8_t*)(data + nal_start), nal_size, h264_info);
                    if (result != 0) {
                        std::cout << "H26xParseSPS::h264Parse failed." << std::endl;
                        Log(Tool::Warning, "H26xParseSPS::h264Parse failed!!!");
                        return false;
                    }

                    codecpar->width = h264_info.width;
                    codecpar->height = h264_info.height;
                    codecpar->profile = h264_info.profile_idc;
                    codecpar->level = h264_info.level_idc;
                    bFindSPS = true;
                }
                else if (nal_type == 8) {
                    bFindPPS = true;
                }

                if (bFindSPS && bFindPPS) {
                    return true;
                }
            }

            offset = next_start;
        }
        else {
            offset++;
        }
    }
    return false;
}

bool CMeadiaPaser::extract_sps_pps_from_packet_hevc(AVCodecParameters* codecpar, const AVPacket* pkt)
{
    const uint8_t* data = pkt->data;
    int size = pkt->size;
    int offset = 0;
    bool bFindVPS = false;
    while (offset < size - 3) {
        // 查找起始码 (00 00 01 或 00 00 00 01)
        if ((data[offset] == 0x00 && data[offset + 1] == 0x00 && data[offset + 2] == 0x01) ||
            (offset < size - 4 && data[offset] == 0x00 && data[offset + 1] == 0x00 &&
                data[offset + 2] == 0x00 && data[offset + 3] == 0x01)) {

            int start_code_len = (data[offset + 2] == 0x01) ? 3 : 4;

            if (data[offset + start_code_len] == 0xba) //ps head
            {
                bNeedDemuxPS = true;
            }

            int nal_start = offset + start_code_len;
            if (nal_start >= size) break;

            uint8_t nal_type = (data[nal_start] >> 1) & 0x3F;  //0x01 0xba  0x01 0xbb 0x01 0xbc

            // 查找下一个起始码
            int next_start = nal_start;
            while (next_start < size - 3) {
                if ((data[next_start] == 0x00 && data[next_start + 1] == 0x00 && data[next_start + 2] == 0x01) ||
                    (next_start < size - 4 && data[next_start] == 0x00 && data[next_start + 1] == 0x00 &&
                        data[next_start + 2] == 0x00 && data[next_start + 3] == 0x01)) {
                    break;
                }
                next_start++;
            }

            int nal_size = (next_start < size) ? (next_start - nal_start) : (size - nal_start);

            if (nal_type == 32 || nal_type == 33 || nal_type == 34) {
                // 跳过已处理的 VPS/SPS/PPS
                if ((nal_type == 32 && bFindVPS) || (nal_type == 33 && bFindSPS) || (nal_type == 34 && bFindPPS)) {
                    offset = next_start;
                    continue;
                }

                // 计算新数据大小（不含填充字节）
                int new_data_size = codecpar->extradata_size + 4 + nal_size;
                uint8_t* new_data = (uint8_t*)av_realloc(codecpar->extradata, new_data_size + AV_INPUT_BUFFER_PADDING_SIZE);
                if (!new_data) return false;

                codecpar->extradata = new_data;
                int write_pos = codecpar->extradata_size;

                // 写入起始码和 NAL 数据
                uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
                memcpy(codecpar->extradata + write_pos, start_code, 4);
                memcpy(codecpar->extradata + write_pos + 4, data + nal_start, nal_size);
                codecpar->extradata_size = new_data_size;

                // 填充末尾
                memset(codecpar->extradata + write_pos + 4 + nal_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

                // 解析 SPS
                if (nal_type == 33) {
                    monk::H26xSPSInfo h265_info;

                    int32_t result = monk::H26xParseSPS::h265Parse((const uint8_t*)(data + nal_start), nal_size, h265_info);
                    if (result != 0) {
                        std::cout << "H26xParseSPS::h265Parse failed." << std::endl;
                        return false;
                    }

                    codecpar->level = h265_info.level_idc;
                    codecpar->width = h265_info.width;
                    codecpar->height = h265_info.height;
                    codecpar->profile = h265_info.profile_idc;

                    bFindSPS = true;
                }
                else if (nal_type == 34) {
                    bFindPPS = true;
                }

                if (bFindSPS && bFindPPS) {
                    return true;
                }
            }

            offset = next_start;
        }
        else {
            offset++;
        }
    }
    return false;
}