/* ��׼��ͷ�ļ� */
#include <string>

/* ˽�п�ͷ�ļ� */
#include "ps_struct.h"
#include "ps_demux.h"

CParsePS::CParsePS() :
	es_cb_(NULL),
	user_param_(NULL),
	es_video_data_index_(0),
	es_audio_data_index_(0),
	is_begin_parse_(false),
	video_stream_id_(-1),
	audio_stream_id_(-1),
	reading_type_(PS_ES_TYPE_INVALID),
	ps_data_index_(0),
	aac_param_update_(false),
	es_video_data_(NULL),
	es_audio_data_(NULL),
	ps_data_(NULL)
{
	return;
}

CParsePS::~CParsePS()
{
	if (NULL != es_video_data_)
	{
		free(es_video_data_);
		es_video_data_ = NULL;
	}

	if (NULL != es_audio_data_)
	{
		free(es_audio_data_);
		es_audio_data_ = NULL;
	}

	if (NULL != ps_data_)
	{
		free(ps_data_);
		ps_data_ = NULL;
	}
}

/* ����ES���ݻص��������û����� */
int CParsePS::set_es_callback(es_callback es_cb, void* user_param)
{
	es_cb_ = es_cb;
	user_param_ = user_param;

	return 0;
}

/* ��ʼ�����������뱾�ػ����ڴ� */
int CParsePS::init_parse()
{
	es_video_data_ = (unsigned char*)malloc(ES_BUFFER);
	if (NULL == es_video_data_)
	{
		return -1;
	}

	es_audio_data_ = (unsigned char*)malloc(ES_BUFFER);
	if (NULL == es_audio_data_)
	{
		return -1;
	}

	ps_data_ = (unsigned char*)malloc(PS_BUFFER);
	if (NULL == ps_data_)
	{
		return -1;
	}

	return 0;
}

/* ���ps���ݿ�ʼ���н��� */
int CParsePS::put_pkt_data(unsigned char* pkt_data, int pkt_data_len)
{
	/* ��μ�� */
	if (!pkt_data || 0 >= pkt_data_len)
	{
		return -1;
	}

	/* ��ĳһ������ psϵͳͷ �� psͷ��ʼ�������ݣ�Ŀ���ǽ���֮ǰҪ���ȴ�ps system map ֮�л�õ�����Ƶ���������ͺ�����Ƶ��id */
	if (!is_begin_parse_)
	{
		/* ���ҵ�һ��ps header */
		unsigned char* ps_header_pos = seek_ps_header(pkt_data, pkt_data_len);
		if (NULL == ps_header_pos)
			return -1;

		while (NULL != ps_header_pos)
		{
			/* ѭ��Ѱ�Һ���system map��ps packet */
			bool is_has_system_map = is_ps_has_system_map(ps_header_pos, pkt_data_len);
			if (true == is_has_system_map)
			{
				break;
			}

			ps_header_pos = seek_ps_header(ps_header_pos + 4, pkt_data_len);
		}

		/* ��ǰ�������֮��û���ҵ�����system map �� ps packet��ֱ�ӷ���ʧ��*/
		if (NULL == ps_header_pos)
		{
			return -1;
		}

		int ps_all_header_len = update_ps_param(ps_header_pos, pkt_data_len);		//��ȡ stream_type, stream_id
		if (-1 == ps_all_header_len)
		{
			return -1;
		}

		/* �����Ϣ��ȡ�ɹ�����ʼ����ps���� */
		pkt_data = ps_header_pos;
		is_begin_parse_ = true;
	}

	while (pkt_data_len > 0)
	{
		int ps_header_pos_len = pkt_data_len;
		unsigned char* ps_header_pos = seek_ps_header(pkt_data, ps_header_pos_len);
		if (NULL == ps_header_pos)			//��������û��һ��psͷ
		{
			if (PS_BUFFER < ps_data_index_ + pkt_data_len)
			{
				return -1;
			}

			memcpy(ps_data_ + ps_data_index_, pkt_data, pkt_data_len);
			ps_data_index_ += pkt_data_len;
			return 0;
		}
		else if (ps_header_pos == pkt_data)	//���ݰ���psͷ��ͷ
		{
			int ps_header_pos_next_len = ps_header_pos_len - 4;
			unsigned char* ps_header_pos_next = seek_ps_header(ps_header_pos + 4, ps_header_pos_next_len);	//Ѱ�Ҵ˶�����֮����һ��psͷ��λ��
			if (NULL == ps_header_pos_next)				//�������ݲ�������һ��psͷ
			{
				/* �ȴ���ǰps����֮�е�ps���� */
				if (0 != ps_data_index_)
				{
					int err_code = process_ps_data();
					if (-1 == err_code)
					{
						memset(ps_data_, 0, PS_BUFFER);
						ps_data_index_ = 0;
					}
				}

				/* Ȼ���ٿ����������ݵ�ps����֮�� */
				memcpy(ps_data_ + ps_data_index_, pkt_data, pkt_data_len);
				ps_data_index_ += pkt_data_len;
				return 0;
			}
			else										//������һ��psͷ
			{
				/* �ȴ���ǰps����֮�е�ps���� */
				if (0 != ps_data_index_)
				{
					int err_code = process_ps_data();
					if (-1 == err_code)
					{
						memset(ps_data_, 0, PS_BUFFER);
						ps_data_index_ = 0;
					}
				}

				/* ��������ps��ͷ֮������ݵ�ps����֮�� */
				int cpy_data_len = ps_header_pos_len - ps_header_pos_next_len;
				memcpy(ps_data_ + ps_data_index_, ps_header_pos, cpy_data_len);
				ps_data_index_ += cpy_data_len;

				//����������ת����һ��ps��ͷ
				pkt_data = ps_header_pos_next;
				pkt_data_len = ps_header_pos_next_len;

				continue;
			}
		}
		else								//psͷ�����ݰ��м�
		{
			/* ����psͷ֮ǰ������ */
			int cpy_data_len = pkt_data_len - ps_header_pos_len;
			if (PS_BUFFER < ps_data_index_ + cpy_data_len)
			{
				return -1;
			}
			memcpy(ps_data_ + ps_data_index_, pkt_data, cpy_data_len);
			ps_data_index_ += cpy_data_len;

			/* ������ת����һ��psͷ */
			pkt_data = ps_header_pos;
			pkt_data_len = ps_header_pos_len;
		}
	}

	return 0;
}

int CParsePS::process_ps_data()
{
	/* ���쳣���ݽ��й��� */
	if (!ps_data_ || ps_data_index_ <= 0)
	{
		return -1;
	}

	/* ����֮�в���һ����ɵ�ps���ݰ���ʱ������ݽ��й��� */
	if (!ps_header_begin(ps_data_, ps_data_index_))
	{
		return -1;
	}


	unsigned char* ps_data = ps_data_;
	int ps_data_len = ps_data_index_;

	/* ����ps���� */
	int ps_all_header_len = update_ps_param(ps_data, ps_data_len);
	if (-1 == ps_all_header_len)
		return -1;

	/* ����psͷ */
	ps_data += ps_all_header_len;
	ps_data_len -= ps_all_header_len;

	bool is_update_video_pts = true;	//��Ƶ����ֶ��pes�����з��ͣ����ڽ�������һ��pes����ʱ���ȡpts

	/* ����ps payload, ÿһ�δ���϶�Ҫ��pesͷ��ʼ */
	while (0 < ps_data_len)
	{
		/* ����pes����*/
		int pes_header_len = update_pes_param(ps_data, ps_data_len, is_update_video_pts);
		if (-1 == pes_header_len)
		{
			return -1;
		}

		/* ����pesͷ */
		ps_data += pes_header_len;
		ps_data_len -= pes_header_len;

		/* Ѱ����һ��pes��λ�� */
		int pes_header_next_len = ps_data_len;
		unsigned char* pes_header_next = seek_pes_header(ps_data, pes_header_next_len);
		if (NULL == pes_header_next)							//û����һ��pes���ݰ���
		{
			/* ֱ�ӿ�����ǰ���� */
			cpy_es_data_to_memory(ps_data, ps_data_len);
			break;
		}
		else												//�ҵ�����һ��pes���ݰ�
		{
			/* ��������pes֮������� */
			int cpy_data_len = ps_data_len - pes_header_next_len;
			cpy_es_data_to_memory(ps_data, cpy_data_len);

			/* ��ת����һ��pes��λ�� */
			ps_data = pes_header_next;
			ps_data_len = pes_header_next_len;

			is_update_video_pts = false;
		}
	}

	/* �������ݻص����������ϻص���Ƶ���� */
	int err_code = touch_es_callback();
	if (-1 == err_code)
	{
		return -1;
	}

	memset(ps_data_, 0, PS_BUFFER);
	ps_data_index_ = 0;

	return 0;
}

/* ���ϲ㴥���������ص����� */
int CParsePS::touch_es_callback()
{
	if (-1 != video_stream_id_ && 0 != es_video_data_index_)	//����֮�б�����Ҫ�����ݷ��ɻص�
	{
		unsigned char* es_all_v = es_video_data_;
		int es_all_len = es_video_data_index_;

		do
		{
			int remain_data_len_f = es_all_len;
			unsigned char* nalu_head_first = find_nalu_startcode(es_all_v, remain_data_len_f);
			if (NULL == nalu_head_first)
			{
				break;
			}

			while (5 < remain_data_len_f)	//nalu head len
			{
				/* ���˵������ĵ����� */
				if (0x09 == nalu_head_first[4])	//����������0x0000000109�ָ���
				{
					nalu_head_first += 4;
					remain_data_len_f -= 4;
					nalu_head_first = find_nalu_startcode(nalu_head_first, remain_data_len_f);
					if (NULL == nalu_head_first)
					{
						break;
					}
				}

				int remain_data_len_s = remain_data_len_f - 4;
				unsigned char* nalu_head_second = find_nalu_startcode(nalu_head_first + 4, remain_data_len_s);
				if (NULL == nalu_head_second)
				{
					/* ������Ƶ���ص� */
					es_param_.video_param.is_i_frame = es_is_i_frame(nalu_head_first);
					es_param_.video_param.frame_type = get_video_frame_type(nalu_head_first);
					es_param_.es_type = PS_ES_TYPE_VIDEO;
					if (NULL != es_cb_)
						es_cb_(nalu_head_first, remain_data_len_f, es_param_, user_param_);

					break;
				}
				else
				{
					/* �ȴ�����Ƶ���ص���Ȼ�����һ֡��ʼ���� */
					es_param_.video_param.is_i_frame = es_is_i_frame(nalu_head_first);
					es_param_.video_param.frame_type = get_video_frame_type(nalu_head_first);
					es_param_.es_type = PS_ES_TYPE_VIDEO;

					int touch_data_len = remain_data_len_f - remain_data_len_s;
					if (NULL != es_cb_)
						es_cb_(nalu_head_first, touch_data_len, es_param_, user_param_);

					nalu_head_first = nalu_head_second;
					remain_data_len_f = remain_data_len_s;
				}

			}

		} while (false);

		memset(es_video_data_, 0, ES_BUFFER);
		es_video_data_index_ = 0;
		es_param_.video_param.is_i_frame = false;
	}

	if (-1 != audio_stream_id_ && 0 != es_audio_data_index_)
	{
		/* ������Ƶ���ص� */
		es_param_.es_type = PS_ES_TYPE_AUDIO;

		if (es_cb_)
			es_cb_(es_audio_data_, es_audio_data_index_, es_param_, user_param_);

		memset(es_audio_data_, 0, ES_BUFFER);
		es_audio_data_index_ = 0;
	}

	return 0;
}

/* У��һ�������Ƿ��� ps header(0x000001BA)��ʼ */
bool CParsePS::ps_header_begin(unsigned char* data, int data_len)
{
	if (!data || data_len <= 0)
	{
		return false;
	}

	if ((0x00 == data[0]) && (0x00 == data[1]) && (0x01 == data[2]) && (0xBA == data[3]))
	{
		return true;
	}

	return false;
}

/* Ѱ��һ������֮��ps header(0x000001BA)��λ�� */
unsigned char* CParsePS::seek_ps_header(unsigned char* data, int& data_len)
{
	if (!data || data_len <= 0)
		return NULL;

	data += 3;
	data_len -= 3;

	while (data_len > 0)
	{
		if (*data == 0xBA)
		{
			if (*(data - 3) == 0x00 && *(data - 2) == 0x00 && *(data - 1) == 0x01)
			{
				data_len += 3;
				return data - 3;
			}

			data += 4;
			data_len -= 4;

			continue;
		}

		data += 1;
		data_len -= 1;
	}

	return NULL;
}

/* Ѱ��һ������֮��pes header(0x000001 stream id)��λ�� */
unsigned char* CParsePS::seek_pes_header(unsigned char* data, int& data_len)
{
	if (!data || data_len <= 0)
		return NULL;

	data += 3;
	data_len -= 3;

	while (data_len > 0)
	{
		if (*data == video_stream_id_ || *data == audio_stream_id_)
		{
			if (*(data - 3) == 0x00 && *(data - 2) == 0x00 && *(data - 1) == 0x01)
			{
				data_len += 3;
				return data - 3;
			}

			data += 4;
			data_len -= 4;

			continue;
		}
		else
		{
			data += 1;
			data_len -= 1;
		}
	}

	return NULL;
}

/* У��һ��ps����֮���Ƿ���ps system header(0x000001BB) */
bool CParsePS::is_ps_has_system_header(unsigned char* ps_data, int ps_data_len)
{
	if (!ps_data || ps_data_len <= 0)
		return false;

	/* �����system headerӦ�ý�����ps header֮������ط�����ps header len  */
	int pack_stuffing_length = ps_data[13] & 0x07;	//psͷ�������ֽڳ���
	int ps_header_len = 14 + pack_stuffing_length;	//psͷ����ֽ�֮ǰһ��14���ֽ�

	/* У��ps header֮������ŵ������Ƿ�Ϊ system header */
	if (ps_data[ps_header_len] == 0x00 && ps_data[ps_header_len + 1] == 0x00 && ps_data[ps_header_len + 2] == 0x01 && ps_data[ps_header_len + 3] == 0xBB)
		return true;

	return false;
}

/* У��һ��ps����֮���Ƿ���ps system map(0x000001BC) */
bool CParsePS::is_ps_has_system_map(unsigned char* ps_data, int ps_data_len)
{
	if (!ps_data || ps_data_len <= 0)
		return false;

	/* ����ps header */
	int pack_stuffing_length = ps_data[13] & 0x07;	//psͷ�������ֽڳ���
	int ps_header_len = 14 + pack_stuffing_length;	//psͷ����ֽ�֮ǰһ��14���ֽ�

	/* ���� system header */
	int ps_system_header_len = 0;
	bool is_sys_h = is_ps_has_system_header(ps_data, ps_data_len);
	if (!is_sys_h)			//������ϵͳͷ
	{
		ps_system_header_len = 0;
	}
	else					//����ϵͳͷ
	{
		ps_system_header_len = (ps_data[ps_header_len + 4] << 8) + ps_data[ps_header_len + 5] + 6;	//system header����
	}

	/* У��ps system map�Ƿ���� */
	unsigned char* ps_sytem_map_data = ps_data + ps_header_len + ps_system_header_len;
	int ps_system_map_len_before = ps_data_len - ps_header_len - ps_system_header_len;
	bool is_sys_map = is_system_map(ps_sytem_map_data, ps_system_map_len_before);

	return is_sys_map;
}

bool is_hevc(uint8_t* data) {
	// ������ʼ�� 00 00 01 �� 00 00 00 01
	uint32_t start_code = (data[0] << 16) | (data[1] << 8) | data[2];
	if (start_code != 0x000001) {
		start_code = (start_code << 8) | data[3];
		if (start_code != 0x00000001) return false;
		data += 4;  // ����4�ֽ���ʼ��
	}
	else {
		data += 3;  // ����3�ֽ���ʼ��
	}

	// ���NAL��Ԫ���ͣ�HEVCΪ2�ֽ�ͷ��
	uint8_t nal_type = (data[0] >> 1) & 0x3F;
	return (nal_type == 32 || nal_type == 33 || nal_type == 34);  // VPS/SPS/PPS
}

/* ����ps��֮�� stream encode type, stream id ����ֵΪps header + ps system header + ps system map���� */
int CParsePS::update_ps_param(unsigned char* ps_data, int ps_data_len)
{
	/* ���У�� */
	if (!ps_data || ps_data_len <= 0)
		return -1;

	/* ps header ���� */
	int pack_stuffing_length = ps_data[13] & 0x07;	//psͷ�������ֽڳ���
	int ps_header_len = 14 + pack_stuffing_length;	//psͷ����ֽ�֮ǰһ��14���ֽ�

	/* ps system mheader ���� */
	int ps_system_header_len = 0;
	bool is_sys_h = is_ps_has_system_header(ps_data, ps_data_len);
	if (!is_sys_h)			//������ϵͳͷ
	{
		ps_system_header_len = 0;
	}
	else					//����ϵͳͷ
	{
		ps_system_header_len = (ps_data[ps_header_len + 4] << 8) + ps_data[ps_header_len + 5] + 6;	//system header����
	}

	/* ps system map ���� */
	unsigned char* ps_sytem_map_data = ps_data + ps_header_len + ps_system_header_len;
	int ps_system_map_len_before = ps_data_len - ps_header_len - ps_system_header_len;
	bool is_sys_map = is_system_map(ps_sytem_map_data, ps_system_map_len_before);
	if (false == is_sys_map)
	{
		return ps_header_len + ps_system_header_len;
	}

	int ps_system_map_es_len_index = (ps_sytem_map_data[8] << 8) + ps_sytem_map_data[9] + 10;
	int ps_system_map_es_info_index = ps_system_map_es_len_index + 2;

	/* ����ѭ������map֮�н��� stream num , stream encode type, stream id */
	int cycle_len = (ps_sytem_map_data[ps_system_map_es_len_index] << 8) + ps_sytem_map_data[ps_system_map_es_len_index + 1];
	while (cycle_len > 0)
	{
		if (0x1B == ps_sytem_map_data[ps_system_map_es_info_index])			//h264
		{
			video_stream_id_ = ps_sytem_map_data[ps_system_map_es_info_index + 1];
			es_param_.video_param.video_encode_type = PS_VIDEO_ENCODE_H264;
		}
		else if (0x24 == ps_sytem_map_data[ps_system_map_es_info_index])
		{
			video_stream_id_ = ps_sytem_map_data[ps_system_map_es_info_index + 1];;
			es_param_.video_param.video_encode_type = PS_VIDEO_ENCODE_H265;
		}
		else if (0x90 == ps_sytem_map_data[ps_system_map_es_info_index])		//g711 ͨ�����Ͳ������ǹ̶�ֵ
		{
			audio_stream_id_ = ps_sytem_map_data[ps_system_map_es_info_index + 1];
			es_param_.audio_param.audio_encode_type = PS_AUDIO_ENCODE_PCMA;
			es_param_.audio_param.channels = 1;
			es_param_.audio_param.samples_rate = 8000;
		}
		else if (0x0F == ps_sytem_map_data[ps_system_map_es_info_index])	//AAC pes param��ʱ�����µ�
		{
			audio_stream_id_ = ps_sytem_map_data[ps_system_map_es_info_index + 1];
			es_param_.audio_param.audio_encode_type = PS_AUDIO_ENCODE_AAC;
		}else if (0xBD == ps_sytem_map_data[ps_system_map_es_info_index])
		{  // ˽����
			uint8_t* pes_payload = ps_sytem_map_data + ps_system_map_es_info_index;
			if (is_hevc(pes_payload)) {  // ����Ƿ�ΪHEVC
				video_stream_id_ = 0xBD;
				es_param_.video_param.video_encode_type = PS_VIDEO_ENCODE_H265;
			}
		}
		else
		{
			/* unknow stream type */
		}

		int high_byte_len = ps_sytem_map_data[ps_system_map_es_info_index + 2] << 8;
		int low_byte_len = ps_sytem_map_data[ps_system_map_es_info_index + 3];

		ps_system_map_es_info_index += high_byte_len + low_byte_len + 2 + 2;
		cycle_len -= (high_byte_len + low_byte_len + 2 + 2);
	}

	int ps_system_map_len = (ps_sytem_map_data[4] << 8) + ps_sytem_map_data[5] + 6;

	return ps_header_len + ps_system_header_len + ps_system_map_len;
}

/* ����pes���������� pes header len */
int CParsePS::update_pes_param(unsigned char* pes_data, int pes_data_len, bool is_updata_v_pts)
{
	if (!pes_data || pes_data_len < 9)
		return -1;

	if (!(pes_data[0] == 0x00 && pes_data[1] == 0x00 && pes_data[2] == 0x01))	//��pes����
		return -1;

	if (video_stream_id_ == pes_data[3])	//��Ƶ�������������ͣ���ȡpts
	{
		reading_type_ = PS_ES_TYPE_VIDEO;
		if (is_updata_v_pts)
		{
			es_param_.video_param.pts = get_pts(pes_data, pes_data_len);
		}
	}
	else if (audio_stream_id_ == pes_data[3])
	{
		/* ����aac �������� aac data adts header, ���з��������ʺ�ͨ����, aac_param_update_ ��ʶ�Ƿ��Ѿ������� aac �Ĳ����ʺ�ͨ�����������ɹ����˾Ͳ��ٽ��� */
		if (!aac_param_update_ && es_param_.audio_param.audio_encode_type == PS_AUDIO_ENCODE_AAC)
		{
			unsigned int es_begin_distance = pes_data[8] + 9;
			if (es_begin_distance + 2 > (unsigned int)pes_data_len)		//���Ȳ����������ݰ�����
				return -1;

			if (!(0xFF == (pes_data[es_begin_distance] & 0xFF) && 0xF0 == (pes_data[es_begin_distance] & 0xF0)))		//adts startcode
			{
				return -1;
			}

			/* adts ��ȡ������ */
			int frequency = (pes_data[es_begin_distance + 2] & 0x3C) >> 2;
			if (0 == frequency)
			{
				es_param_.audio_param.samples_rate = 96000;
			}
			else if (1 == frequency)
			{
				es_param_.audio_param.samples_rate = 88200;
			}
			else if (2 == frequency)
			{
				es_param_.audio_param.samples_rate = 64000;
			}
			else if (3 == frequency)
			{
				es_param_.audio_param.samples_rate = 48000;
			}
			else if (4 == frequency)
			{
				es_param_.audio_param.samples_rate = 44100;
			}
			else if (5 == frequency)
			{
				es_param_.audio_param.samples_rate = 32000;
			}
			else if (6 == frequency)
			{
				es_param_.audio_param.samples_rate = 24000;
			}
			else if (7 == frequency)
			{
				es_param_.audio_param.samples_rate = 22050;
			}
			else if (8 == frequency)
			{
				es_param_.audio_param.samples_rate = 16000;
			}
			else if (9 == frequency)
			{
				es_param_.audio_param.samples_rate = 12000;
			}
			else if (10 == frequency)
			{
				es_param_.audio_param.samples_rate = 11025;
			}
			else if (11 == frequency)
			{
				es_param_.audio_param.samples_rate = 8000;
			}
			else if (12 == frequency)
			{
				es_param_.audio_param.samples_rate = 7350;
			}
			else
			{
				return -1;
			}

			/* adts ��ȡͨ���� */

			int channles = ((pes_data[es_begin_distance + 2] & 0x01) << 2) + ((pes_data[es_begin_distance + 3] & 0xC0) >> 6);
			if (1 == channles)
			{
				es_param_.audio_param.channels = 1;
			}
			else if (2 == channles)
			{
				es_param_.audio_param.channels = 2;
			}
			else if (3 == channles)
			{
				es_param_.audio_param.channels = 3;
			}
			else if (4 == channles)
			{
				es_param_.audio_param.channels = 4;
			}
			else if (5 == channles)
			{
				es_param_.audio_param.channels = 5;
			}
			else if (6 == channles)
			{
				es_param_.audio_param.channels = 6;
			}
			else if (7 == channles)
			{
				es_param_.audio_param.channels = 8;
			}
			else
			{
				return -1;
			}

			aac_param_update_ = true;
		}

		/* ��Ƶ�������������ͣ���ȡpts */
		reading_type_ = PS_ES_TYPE_AUDIO;
		es_param_.audio_param.pts = get_pts(pes_data, pes_data_len);
	}
	else
	{
		return -1;
	}

	return pes_data[8] + 9;
}

/* ��ȡpts */
__int64 CParsePS::get_pts(unsigned char* pes_data, int pes_data_len)
{
	pes_data += 9;		//ƫ����pts����λ��
	return (unsigned __int64)(*pes_data & 0x0e) << 29 |
		(AV_RB16(pes_data + 1) >> 1) << 15 |
		AV_RB16(pes_data + 3) >> 1;
}

/* ��ȡdts */
__int64 CParsePS::get_dts(unsigned char* pes_data, int pes_data_len)
{
	pes_data += 14;		//ƫ����pts����λ��
	return (unsigned __int64)(*pes_data & 0x0e) << 29 |
		(AV_RB16(pes_data + 1) >> 1) << 15 |
		AV_RB16(pes_data + 3) >> 1;
}

/* ����һ������֮�е�0x00000001λ�ã�����ֵΪλ��ָ�룬�������data_lenΪʣ�����ݳ��� */
unsigned char* CParsePS::find_nalu_startcode(unsigned char* data, int& data_len)
{
	if (!data || data_len < 4)
		return NULL;

	data += 3;
	data_len -= 3;
	while (data_len >= 0)
	{
		if (*data == 0x01)
		{
			if (0x00 == *(data - 1) && 0x00 == *(data - 2) && 0x00 == *(data - 3))
			{
				data_len += 3;	//��Ϊ����λ����ǰ�ƶ�3���ֽڣ�����data_lenӦ�ü�3
				return data - 3;
			}
			else
			{
				data += 4;
				data_len -= 4;
				continue;
			}
		}
		data += 1;
		data_len -= 1;
	}

	return NULL;
}

/* ����һ������֮�е�0x000001λ�ã�����ֵΪλ��ָ�룬�������data_lenΪʣ�����ݳ��� */
unsigned char* CParsePS::find_nalu_startcode2(unsigned char* data, int& data_len)
{
	if (!data || data_len < 3)
		return NULL;

	data += 2;
	data_len -= 2;

	while (data_len >= 0)
	{
		if (*data == 0x01)
		{
			if (0x00 == *(data - 1) && 0x00 == *(data - 2))
			{
				data_len += 2;	//��Ϊ����λ����ǰ�ƶ�3���ֽڣ�����data_lenӦ�ü�3
				return data - 2;
			}
			else
			{
				data += 3;
				data_len -= 3;
				continue;
			}
		}
		data += 1;
		data_len -= 1;
	}

	return NULL;
}

/* ����es���ݵ�����es���� */
int CParsePS::cpy_es_data_to_memory(unsigned char* es_data, int es_data_len)
{
	if (reading_type_ == PS_ES_TYPE_VIDEO)
		return cpy_es_data_to_video_memory(es_data, es_data_len);
	else if (reading_type_ == PS_ES_TYPE_AUDIO)
		return cpy_es_data_to_audio_memory(es_data, es_data_len);
	else
		return -1;
}

/* ����es���ݵ���Ƶes���� */
int CParsePS::cpy_es_data_to_video_memory(unsigned char* es_data, int es_data_len)
{
	if (!es_data || es_data_len <= 0)
		return -1;

	int cpy_data_len = 0;

	if (es_data_len <= ES_BUFFER - es_video_data_index_)
	{
		memcpy(es_video_data_ + es_video_data_index_, es_data, es_data_len);
		es_video_data_index_ += es_data_len;
		cpy_data_len += es_data_len;

		return cpy_data_len;
	}

	return cpy_data_len;
}

/* ����es���ݵ���Ƶ���ݻ��� */
int CParsePS::cpy_es_data_to_audio_memory(unsigned char* es_data, int es_data_len)
{
	if (!es_data || es_data_len <= 0)
		return -1;

	if (es_data_len <= ES_BUFFER - es_audio_data_index_)
	{
		memcpy(es_audio_data_ + es_audio_data_index_, es_data, es_data_len);
		es_audio_data_index_ += es_data_len;
		return es_data_len;
	}

	return -1;
}

/* I֡�ж� */
bool CParsePS::es_is_i_frame(unsigned char* es_data)
{
	if(!ps_as_hevc)
		return ((es_data[4] & 0x1F) == 5 || (es_data[4] & 0x1F) == 2 || (es_data[4] & 0x1F) == 7 || (es_data[4] & 0x1F) == 8) ? true : false;
	else
		return (((es_data[4] >> 1) & 0x3F) == 33 || ((es_data[4] >> 1) & 0x3F) == 34 || ((es_data[4] >> 1) & 0x3F) == 19 || ((es_data[4] >> 1) & 0x3F) == 20 || ((es_data[4] >> 1) & 0x3F) == 21) ? true : false;
}

/* ��ȡ��Ƶ֡���� */
PS_ESFrameType_E CParsePS::get_video_frame_type(unsigned char* es_data)
{
#if 0
		int type = (es_data[4] >> 1) & 0x3F;

		switch (type) {
		case 0:
			return PS_ES_FRAME_TYPE_DATA;
		case 19:
		case 20:
		case 21:
			return PS_ES_FRAME_TYPE_IDR;
		case 6:
			return PS_ES_FRAME_TYPE_SEI;
		case 33:
			return PS_ES_FRAME_TYPE_SPS;
		case 34:
			return PS_ES_FRAME_TYPE_PPS;
		}
		return PS_ES_FRAME_TYPE_INVALID;
#endif
		// ������ʼ�루00 00 01 �� 00 00 00 01��
		int offset = 0;
		if (es_data[0] == 0x00 && es_data[1] == 0x00 && es_data[2] == 0x01) {
			offset = 3;
		}
		else if (es_data[0] == 0x00 && es_data[1] == 0x00 && es_data[2] == 0x00 && es_data[3] == 0x01) {
			offset = 4;
		}
		uint8_t* nal_start = es_data + offset;

		if (!ps_as_hevc) {  // H.264
			uint8_t nal_type = nal_start[0] & 0x1F;  // ��5λ
			switch (nal_type) {
			case 1:  case 2:  case 3:  case 4:  // ��ͨ֡
				return PS_ES_FRAME_TYPE_DATA;
			case 5:  // IDR ֡
				return PS_ES_FRAME_TYPE_IDR;
			case 6:  // SEI
				return PS_ES_FRAME_TYPE_SEI;
			case 7:  // SPS
				return PS_ES_FRAME_TYPE_SPS;
			case 8:  // PPS
				return PS_ES_FRAME_TYPE_PPS;
			default:
				return PS_ES_FRAME_TYPE_INVALID;
			}
		}
		else {  // HEVC
			//if (offset + 1 >= data_len) return PS_ES_FRAME_TYPE_INVALID;  // ȷ����2�ֽ�NALͷ

			uint8_t nal_type = (nal_start[0] >> 1) & 0x3F;  // ��6λ
			switch (nal_type) {
			case 1:  case 2:  case 3:  case 4:  // ��ͨ֡��B/P֡��
				return PS_ES_FRAME_TYPE_DATA;
			case 19:  // IDR_N_LP
			case 20:  // CRA_NUT
				return PS_ES_FRAME_TYPE_IDR;
			case 32:  // VPS
				return PS_ES_FRAME_TYPE_VPS;  // ����չö������
			case 33:  // SPS
				return PS_ES_FRAME_TYPE_SPS;
			case 34:  // PPS
				return PS_ES_FRAME_TYPE_PPS;
			case 39:  // SEI
				return PS_ES_FRAME_TYPE_SEI;
			default:
				return PS_ES_FRAME_TYPE_INVALID;
			}
		}
}

/* У��һ�������Ƿ���pes header(0x000001 stream id)��ʼ */
bool CParsePS::is_pes_begin(unsigned char* data, int data_len)
{
	if (!data || data_len <= 0)
		return false;

	/* ��Ƶpes */
	if ((0x00 == data[0]) && (0x00 == data[1]) && (0x01 == data[2]) && (video_stream_id_ == data[3]))
		return true;

	/* ��Ƶpes */
	if ((0x00 == data[0]) && (0x00 == data[1]) && (0x01 == data[2]) && (audio_stream_id_ == data[3]))
		return true;

	return false;
}

/* У��һ�������Ƿ���ps system map(0x000001BC)��ʼ */
bool CParsePS::is_system_map(unsigned char* data, int data_len)
{
	if (!data || data_len <= 3)
		return false;

	if ((0x00 == data[0]) && (0x00 == data[1]) && (0x01 == data[2]) && (0xBC == data[3]))
	{
		return true;
	}

	return false;
}

/* ����audio_stream_id�Ƿ�õ�������У������֮���Ƿ�����Ƶ���� */
bool CParsePS::has_audio_stream()
{
	if (-1 == audio_stream_id_)
		return false;

	return true;
}