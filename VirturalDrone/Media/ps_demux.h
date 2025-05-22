#pragma once
#include "ps_struct.h"

/* �⸴��ps�������� */
class CParsePS
{
public:
	CParsePS();
	~CParsePS();

	/* ���ýӿ�es���������ݻص����� */
	int set_es_callback(es_callback es_cb, void* user_param);

	/* ��ʼ���⸴�� */
	int init_parse();

	/* ���ps���� */
	int put_pkt_data(unsigned char* pkt_data, int pkt_data_len);

	/* ps��֮���Ƿ�����Ƶ���� */
	bool has_audio_stream();

	/* ����Ϊhevc�����ʽ��ps���� Ĭ��h264�����ʽ*/
	void set_hevc_ps( bool bSet) { ps_as_hevc = bSet; }

protected:
private:

	/* Ѱ��һ������֮��psͷ��λ�ã�����ֵΪpsͷ��ָ�룬�������data_lenΪ��psͷ��ʼ��ʣ�����ݳ��� */
	unsigned char* seek_ps_header(unsigned char* data, int& data_len);

	/* Ѱ��һ������֮��pesͷ��λ�ã�����ֵΪpesͷ��ָ��, �������data_lenΪpesͷ��ʼ��ʣ�����ݳ��� */
	unsigned char* seek_pes_header(unsigned char* data, int& data_len);

	/* �ж�һ�������Ƿ�����Ƶ������Ƶpesͷ��ʼ */
	bool is_pes_begin(unsigned char* data, int data_len);

	/* �ж�һ��ps����֮���Ƿ���ps system header */
	bool is_ps_has_system_header(unsigned char* ps_data, int ps_data_len);

	/* �ж�һ��ps����֮���Ƿ���ps system map */
	bool is_ps_has_system_map(unsigned char* ps_data, int ps_data_len);

	/* ��һ��ps����֮�з�������Ƶ�������͵���Ϣ������ֵΪ����psͷ�ĳ��� */
	int update_ps_param(unsigned char* ps_data, int ps_data_len);

	/* ��һ��pes���ݷ�����pes�������͡���Ƶ�����ʺ�ͨ��������Ϣ */
	int update_pes_param(unsigned char* pes_data, int pes_data_len, bool is_updata_v_pts);	//����ֵΪ����pesͷ�ĳ���

	/* ����һ��es���ݵ�����es����֮�� */
	int cpy_es_data_to_memory(unsigned char* es_data, int es_data_len);

	/* ����һ����Ƶes���ݵ�������Ƶes���ݻ���֮�� */
	int cpy_es_data_to_video_memory(unsigned char* es_data, int es_data_len);

	/* ����һ����Ƶes���ݵ�������Ƶes���ݻ���֮�� */
	int cpy_es_data_to_audio_memory(unsigned char* es_data, int es_data_len);

	/* ��ȡһ��pes��ͷ֮�е�pts */
	__int64 get_pts(unsigned char* pes_data, int pes_data_len);

	/* ��ȡһ��pes��ͷ֮�е�dts */
	__int64 get_dts(unsigned char* pes_data, int pes_data_len);

	/* Ѱ��һ������֮��0x00000001��ʼ�뿪ʼ��λ��(h264 startcode)�� ����ֵΪλ��ָ�룬�������data_lenΪʣ�����ݳ��� */
	unsigned char* find_nalu_startcode(unsigned char* data, int& data_len);

	/* Ѱ��һ������֮��0x000001��ʼ�뿪ʼ��λ�ã� ����ֵΪλ��ָ�룬�������data_lenΪʣ�����ݳ��� */
	unsigned char* find_nalu_startcode2(unsigned char* data, int& data_len);

	/* �ж�һ��es��Ƶ֡�����Ƿ�Ϊ�ؼ�֡ */
	bool es_is_i_frame(unsigned char* es_data);

	/* ��ȡ��Ƶ֡���� */
	PS_ESFrameType_E get_video_frame_type(unsigned char* es_data);

	/* ����һ��������ps���ݰ����� */
	int process_ps_data();

	/* ���ϲ㴥��es�������ص����� */
	int touch_es_callback();

	/* �ж�һ�������Ƿ���ps header��ʼ */
	bool ps_header_begin(unsigned char* data, int data_len);

	/* �ж�һ�������Ƿ���ps system map��ʼ */
	bool is_system_map(unsigned char* data, int data_len);

	/* ������û����ݻص����û����� */
	es_callback es_cb_;
	void* user_param_;

	PS_ESParam_S es_param_;				/* ��ǰps����Ƶ���ݵ�es ���� */
	bool is_begin_parse_;				/* �Ƿ��Ѿ��ҵ�psͷ��������صı������� */

	int video_stream_id_;				/* ��ps system map ֮�з��������� ��Ƶpes��startcode id */
	int audio_stream_id_;				/* ��ps system map ֮�з��������� ��Ƶpes��startcode id */

	PS_ESType_E reading_type_;			/* ��ǰ���ڶ�ȡ��ES�������� */

	unsigned char* es_video_data_;		/* ������Ƶes���ݻ��� */
	int es_video_data_index_;			/* ������������Ƶes���ݵĳ��� */

	unsigned char* es_audio_data_;		/* ������Ƶes���ݻ��� */
	int es_audio_data_index_;			/* ������������Ƶes���ݵĳ��� */

	unsigned char* ps_data_;			/* ps���ݻ��棬�ӽ��յ�������֮��������ȡ��һ��ps���ݰ�Ϊֹ */
	int ps_data_index_;					/*  */

	bool aac_param_update_;				/* ������Ƶ������aac��������صĲ�������ͨ���������Ƿ��Ѿ���adts֮�з������� */

	bool ps_as_hevc = false;
};