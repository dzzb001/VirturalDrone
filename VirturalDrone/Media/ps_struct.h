#pragma once

#define ES_BUFFER 600000						/* ES���ݰ���������С */
#define PS_BUFFER 1024*1024						/* PS���ݰ���������С */

/* pts�����궨�� */
#ifndef AV_RB16
#   define AV_RB16(x)                        \
	((((const unsigned char*)(x))[0] << 8) | \
	((const unsigned char*)(x))[1])
#endif 

/* ��Ƶ�������Ͷ��� */
typedef enum PS_VideoEncodeType_E
{
	PS_VIDEO_ENCODE_INVALID = -1,
	PS_VIDEO_ENCODE_H264,
	PS_VIDEO_ENCODE_H265
}PS_VideoEncodeType_E;


/* ��Ƶ�������Ͷ��� */
typedef enum PS_AudioEncodeType_E
{
	PS_AUDIO_ENCODE_INVALID = -1,
	PS_AUDIO_ENCODE_PCMA,				//g711a
	PS_AUDIO_ENCODE_AAC					//aac
}PS_AudioEncodeType_E;

/* ES�������Ͷ��� */
typedef enum PS_ESType_E
{
	PS_ES_TYPE_INVALID = -1,
	PS_ES_TYPE_VIDEO,
	PS_ES_TYPE_AUDIO
}EF_ESType;

/* ES����֡���Ͷ��� */
typedef enum PS_ESFrameType_E
{
	PS_ES_FRAME_TYPE_INVALID = -1,
	PS_ES_FRAME_TYPE_DATA = 2,
	PS_ES_FRAME_TYPE_IDR = 5,
	PS_ES_FRAME_TYPE_SEI = 6,
	PS_ES_FRAME_TYPE_SPS = 7,
	PS_ES_FRAME_TYPE_PPS = 8,
	PS_ES_FRAME_TYPE_VPS
}PS_ESFrameType_E;

/* ES�ص�����֮��ES����֮�е���Ƶ�������� */
typedef struct PS_ESVideoParam_S
{
	PS_VideoEncodeType_E video_encode_type;
	bool is_i_frame;
	__int64 pts;
	__int64 dts;
	PS_ESFrameType_E frame_type;

	PS_ESVideoParam_S()
	{
		video_encode_type = PS_VIDEO_ENCODE_INVALID;
		is_i_frame = false;
		pts = 0;
		dts = 0;
		frame_type = PS_ES_FRAME_TYPE_INVALID;
	}
}PS_ESVideoParam_S;

/* ES�ص�����֮��ES����֮�е���Ƶ�������� */
typedef struct PS_ESAudioParam_S
{
	PS_AudioEncodeType_E audio_encode_type;
	int channels;
	int samples_rate;
	__int64 pts;

	PS_ESAudioParam_S()
	{
		audio_encode_type = PS_AUDIO_ENCODE_INVALID;
		channels = 1;									//g711Ĭ������
		samples_rate = 8000;
		pts = 0;
	}
}PS_ESAudioParam_S;

/* ES�ص�����֮��ES�������� */
typedef struct PS_ESParam_S
{
	PS_ESType_E es_type;
	PS_ESVideoParam_S video_param;
	PS_ESAudioParam_S audio_param;

	PS_ESParam_S()
	{
		es_type = PS_ES_TYPE_INVALID;
	}
}PS_ESParam_S;

/**
* @brief ES���ݻص�����
* @param[in] es_data	 ES��������
* @param[in] es_data_len ES�������ݳ���
* @param[in] es_param	 ES���ݲ���
* @param[in] user_param	 �û�����
*
* @return void
*/
typedef void(__stdcall* es_callback)(unsigned char* es_data, int es_data_len, PS_ESParam_S es_param, void* user_param);