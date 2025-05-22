#ifndef DATAUTIL_H
#define DATAUTIL_H

#include <stdint.h>
#include <memory>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#define CALLBACK 
#endif

struct AVFrame;

typedef void(CALLBACK* pfnMsgCallbk)(int iMsg, void* pUserData);

namespace datautil {
	enum Decode_MSG
	{
		//iMsg：消息类型，1 - 播放开始 2 - 播放结束（视频预览无此消息） 3 - 播放异常 10 - 取流开始 11 - 取流结束 12 - 取流异常 50 - 异步时发起取流请求成功 51 - 异步时发起取流请求失败
		DECODE_MSG_PLAY_START = 1,
		DECODE_MSG_PLAY_END = 2,
		DECODE_MSG_PLAY_ERR = 3,
		DECODE_MSG_STREAM_START = 10,
		DECODE_MSG_STREAM_END = 11,
		DECODE_MSG_STREAM_ERR = 12,
	};

	enum DecodetType_t
	{
		VIDEO_NV12 = 0,
		VIDEO_YUV420
	};

	enum DeviceDecoderType_t
	{
		Device_Decoder_Cpu = 0, // 
		Device_Decoder_Intel,
		Device_Decoder_Nvidia, // 
		Device_Decoder_vaapi,
        Device_Decoder_Auto, // 优先 GPU Intel -> GPU Nvidia -> CPU
        Device_Decoder_Unkonw,
	};


	enum Decoder_Thread_Status_Type
	{
		Decoder_Thread_None,
		Decoder_Thread_Run,  //解码线程进行正常解码数据
		Decoder_Thread_Data, //解码线程解完所有的数据退出
		Decoder_Thread_Exit //解码线程退出
	};
	struct avRawData
	{
        uint8_t* _rawdata=nullptr;
        uint8_t*  _data[4]={0};
		uint32_t _ulLineSize[4] = { 0 };
        uint32_t _ulPicHeight=0;
        uint32_t _ulPicWidth=0;
        uint8_t* _ulUserdata=nullptr;
        DecodetType_t  _ulDecodetype=VIDEO_NV12; //
		bool	 _bkey=0;

		bool   _bNeedFree = true; //海康的不需要释放
        
        avRawData() {
			for (int i = 0; i < 4; i++)
			{
				_data[i] = nullptr;
			}
		}
        ~avRawData(){ 
             if(_rawdata)
            {
                delete _rawdata;
                 _rawdata = nullptr;
            }
//             for (int i = 0; i < 4; i++)
//             {
//                 if(_data[i])
//                 {
//                     delete _data[i];
//                 }
//             }
            if(_ulUserdata)
            {
                delete _ulUserdata;
            }

#if 1   //内存拷贝的话需要释放内存，不拷贝直接使用ffmepg 中的内存

			if (_bNeedFree)
			{
				for (int i = 0; i < 4; i++)
				{
					if (_data[i])
					{
						delete[] _data[i];
						_data[i] = nullptr;
					}
				}
			}
#endif
        }

		avRawData(uint8_t* rawdata, uint8_t* data[4], uint32_t ulLineSize[4], uint32_t ulPicHeight, uint32_t ulPicWidth, uint8_t* ulUserdata) 
		{
			this->_rawdata = rawdata;
			//this->data[4] = data[4];
			for (int i = 0; i < 4; i++) {
				this->_data[i] = data[i];
			}
//            std::copy(std::begin(ulLineSize),std::end(ulLineSize),std::begin(_ulLineSize));
            memcpy(this->_ulLineSize,ulLineSize,4*sizeof (uint32_t));
//            this->_ulLineSize[] = ulLineSize[];
			this->_ulPicHeight = ulPicHeight;
			this->_ulPicWidth = ulPicWidth;
			this->_ulUserdata = ulUserdata;
		}
	};
	typedef std::shared_ptr<avRawData> avRawDataPtr;

	enum SDKType_t
	{
		FFmpeg = 0,
		HIK = 1,
		Dahua = 2,
		YuShi = 3
	};


// 	void csleep(uint64_t ms)
// 	{
// 		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
// 	}
}



#endif // !DATAUTIL_H
