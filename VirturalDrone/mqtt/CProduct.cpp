#include "stdafx.h"
#include "CProduct.h"
#include "Tool/cJSON.h"
#include <string>
#include "Tool/XTime.h"

#include "mqtt/async_client.h"
#include "mqtt/client.h"
#include <chrono>
#include "Tool/XStrUtil.h"
#include "Tool/XEvent.h"

#include "Media/MeadiaPaser.h"

#define Log LogN(123)
using namespace std::chrono;

// Gets the current time as the number of milliseconds since the epoch:
// like a time_t with ms resolution.
static uint64_t timestamp()
{
	auto now = system_clock::now();
	auto tse = now.time_since_epoch();
	auto msTm = duration_cast<milliseconds>(tse);
	return uint64_t(msTm.count());
}

CProduct::CProduct() :
	m_bWork(false)
	, m_bJson(true)
	, m_bEnable(false)
	, m_bStartSend(false)
{
}
CProduct::~CProduct()
{
}

bool CProduct::InitMqtt(std::string strId)
{
	Config::CConfig::GetInstance().GetChMqttInfo(strId, m_chNode);

	//判断是否开启mqtt发送
	if (m_chNode.mqtt.strEnable.compare("true"))
	{
		return false;
	}

	SetTopic(m_chNode.mqtt.strTopic.c_str());

	if (m_chNode.mqtt.strDataPath.find(".SRT") >= 0)
	{
		if (!LoadSrt((char*)m_chNode.mqtt.strDataPath.c_str())) {
			Log(Tool::Error, "解析SRT文件失败");
			return false;
		}
	}
	else {
		if (!LoadJson((char*)m_chNode.mqtt.strDataPath.c_str())) {
			Log(Tool::Error, "解析json文件失败");
			return false;
		}
	}

	if (m_chNode.mqtt.strUser.empty())
	{
		Log(Tool::Error, "用户不能为空");
		return false;
	}
	if (m_chNode.mqtt.strPassword.empty())
	{
		Log(Tool::Error, "密码不能为空");
		return false;

	}
	if (m_chNode.mqtt.strSvrAdress.empty())
	{
		Log(Tool::Error, "服务器不能为空");
		return false;
	}
	m_strID = strId;
	return true;
}
bool CProduct::DeInitMqtt()
{
	End();
	return true;
}

void CProduct::ChangeSrtToJson()
{
	std::string strTopic;
	if (!m_strTopic.compare(""))
		strTopic = "thing/product/1581F5FHD22AT00B0094/osd";
	else
		strTopic = m_strTopic.c_str();

	while (m_listFont.size() > 0)
	{
		stuMqMessage message;
		unordered_map<std::string, std::string> mapItem = m_listFont.front();
		unordered_map<std::string, std::string>::iterator itorFind = mapItem.find("createAt");
		m_listFont.pop_front();

		if (itorFind == mapItem.end())
			continue;

		message.strCreatAt = itorFind->second;

		message.strTopic = strTopic.c_str();
		//组装payload

		cJSON* pRoot = cJSON_CreateObject();

		bool bComplite = false;
		do
		{
			cJSON_AddStringToObject(pRoot, "bid", "00000000-0000-0000-0000-000000000000");

			cJSON* pData = cJSON_CreateObject();
			cJSON_AddItemToObject(pRoot, "data", pData);

			cJSON* pItemIndex = cJSON_CreateObject();
			cJSON_AddItemToObject(pData, "66-0-0", pItemIndex);

			unordered_map<std::string, std::string>::iterator itorFind = mapItem.find("gb_pitch");
			if (itorFind == mapItem.end())
			{
				itorFind = mapItem.find("pitch");
				if (itorFind == mapItem.end())
					break;
			}
			float gb_pitch = atof(itorFind->second.c_str());

			cJSON_AddNumberToObject(pItemIndex, "gimbal_pitch", gb_pitch);

			itorFind = mapItem.find("gb_roll");
			if (itorFind == mapItem.end())
			{
				itorFind = mapItem.find("roll");
				if (itorFind == mapItem.end())
					break;
			}
			float gb_roll = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pItemIndex, "gimbal_roll", gb_roll);

			itorFind = mapItem.find("gb_yaw");
			if (itorFind == mapItem.end())
			{
				itorFind = mapItem.find("yaw");
				if (itorFind == mapItem.end())
					break;
			}
			float gb_yaw = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pItemIndex, "gimbal_yaw", gb_yaw);

			cJSON_AddStringToObject(pItemIndex, "payload_index", "66-0-0");
			cJSON_AddNumberToObject(pItemIndex, "version", 1);

			itorFind = mapItem.find("dzoom_ratio");
			if (itorFind == mapItem.end())
				break;
			float dzoom_ratio = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pItemIndex, "zoom_factor", dzoom_ratio);

			cJSON_AddNumberToObject(pData, "attitude_head", gb_yaw);
			cJSON_AddNumberToObject(pData, "attitude_pitch", gb_pitch);
			cJSON_AddNumberToObject(pData, "attitude_roll", gb_roll);

			itorFind = mapItem.find("rel_alt");
			if (itorFind == mapItem.end())
				break;
			float rel_alt = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pData, "elevation", rel_alt);

			itorFind = mapItem.find("abs_alt");
			if (itorFind == mapItem.end())
				break;
			float abs_alt = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pData, "height", abs_alt);

			itorFind = mapItem.find("latitude");
			if (itorFind == mapItem.end())
				break;
			float latitude = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pData, "latitude", latitude);

			itorFind = mapItem.find("longitude");
			if (itorFind == mapItem.end())
				break;
			float longitude = atof(itorFind->second.c_str());
			cJSON_AddNumberToObject(pData, "longitude", longitude);


			cJSON_AddStringToObject(pRoot, "tid", "00000000-0000-0000-0000-000000000000");
			cJSON_AddStringToObject(pRoot, "gateway", "5YSZKAC0020EBX");

			char* pOut = cJSON_Print(pRoot);

			message.strPayload = pOut;

			free(pOut);
			bComplite = true;
		} while (0);

		cJSON_Delete(pRoot);
		
		if(bComplite)
			m_listMessages.push_back(message);
	}
}

bool CProduct::ParserFont(std::string strFontContent, std::list<std::unordered_map<std::string, std::string>> &listFont)
{
	std::unordered_map<std::string, std::string> temp_map;
	std::vector<std::string> vecItem;
	int ret = xbase::XStrUtil::Split(strFontContent, vecItem, "\n");
	if (ret < 5)
		return false;

	temp_map["createAt"] = vecItem[3];

	//拆分无人机属性数据
	std::string str5Line = vecItem[4];
	std::string strSub;
	int nSeek = 0;
	int nPos = -1;
	do
	{
		strSub = str5Line.substr(nSeek);
		nPos = strSub.find("[");
		nSeek += nPos;

		if (nPos < 0)
			break;
		std::string strItem = strSub.substr(nPos + 1);
		nPos = strItem.find("]");
		if (nPos < 0)
			break;
		strItem = strItem.substr(0, nPos);

		nSeek += strItem.size();
		nSeek += 2;//[]

		std::vector<std::string> vecTemp;
		int retSize = xbase::XStrUtil::Split(strItem, vecTemp, ":");


		if (retSize < 2)
			break;
		if (retSize == 2)
		{
			temp_map[vecTemp[0]] = vecTemp[1];
		}
		else {
			vecTemp.clear();
			for (size_t i = 0; i < strItem.size(); i++)
			{
				if (strItem[i] == ':' || strItem[i] == ' ') {
					strItem[i] = '\0';
				}
			}
			bool bGetStart = false;
			bool bEnd = false;
			int nStart = 0;
			int nEnd = 0;
			for (size_t i = 0; i < strItem.size(); i++)
			{
				if (strItem[i] != '\0'&& !bGetStart)
				{
					nStart = i;
					bGetStart = true;
					bEnd = false;
				}
				else if (strItem[i] == '\0' && bGetStart){
					nEnd = i;
					bGetStart = false;
					bEnd = true;
					std::string strIte = strItem.substr(nStart, nEnd);
					vecTemp.push_back(strIte.c_str());
				}
			}
			if (bGetStart && !bEnd)
			{
				std::string strIte = strItem.substr(nStart);
				vecTemp.push_back(strIte.c_str());
			}
			if (vecTemp.size() % 2 == 0)
			{
				for (size_t i = 0; i < vecTemp.size(); i+=2)
				{
					temp_map[vecTemp[i].c_str()] = vecTemp[i + 1].c_str();
				}
			}
		}

	} while (true);
	listFont.push_back(temp_map);
	return true;
}

bool CProduct::LoadSrt(char* sPath)
{
	m_listMessages.clear();
	m_listFont.clear();

	std::string strReadSum;
	std::string strRead;
	char buf[4096];

	FILE* pFile = NULL;

	if (NULL != (pFile = fopen(sPath, "r")))
	{
		size_t nReadCount = 0;
		int nUsed = 0;
		do
		{
			//循环读取数据
			memset(buf, 0, 4096);
			nReadCount = fread(buf, 1, 4096 - 1, pFile);
			strReadSum.append(buf);
			strRead = strReadSum.substr(nUsed);
			//循环解析一帧字幕
			int nSeek = 0;
			do
			{
				std::string strReadTemp = strRead.substr(nSeek);
				//std::string::size_type nEnd;
				int nEnd = strReadTemp.find("\n\n");
				if (nEnd<0)
					break;
				strReadTemp = strReadTemp.substr(0, nEnd + 2);
				nSeek += strReadTemp.size();
				//解析单个字幕数据
				ParserFont(strReadTemp, m_listFont);
			} while (true);
			nUsed += nSeek;
		} while (nReadCount == 4096 - 1);

		fclose(pFile);
		pFile = NULL;
	}

	OutputDebugString("parse complite\n");
	Log(Tool::Debug, "mqtt data parse complite");
	//将srt 数据转换成json payload data
	ChangeSrtToJson();

	OutputDebugString("SrtToJson complite\n");
	Log(Tool::Debug, "SrtToJson complite");
	m_bJson = false;
	return true;
}

bool CProduct::LoadJson(char* sPath)
{
	m_listMessages.clear();
	m_listFont.clear();

	std::string strRead;
	char buf[4096];

	FILE* pFile = NULL;

	if (NULL != (pFile = fopen(sPath, "r")))
	{
		size_t nReadCount = 0;
		do
		{
			memset(buf, 0, 4096);
			nReadCount = fread(buf, 1, 4096 -1, pFile);
			strRead.append(buf);
		} while (nReadCount == 4096-1);

		fclose(pFile);
		pFile = NULL;
	}

	cJSON* pRoot = cJSON_Parse(strRead.c_str());
	if (pRoot)
	{
		m_listMessages.clear();
		for (size_t i = 0; i < cJSON_GetArraySize(pRoot); i++)
		{
			if (i > 0)
				break;
			cJSON* pItem = cJSON_GetArrayItem(pRoot, i);
			if (pItem)
			{
				cJSON* pMessages = cJSON_GetObjectItem(pItem, "messages");
				if (pMessages)
				{
					for (size_t i = 0; i < cJSON_GetArraySize(pMessages); i++)
					{
						cJSON* pMessageItem = cJSON_GetArrayItem(pMessages, i);
						
						stuMqMessage message;
						cJSON* pAt = cJSON_GetObjectItem(pMessageItem, "createAt");
						cJSON* pPayload = cJSON_GetObjectItem(pMessageItem, "payload");
						cJSON* pTopic = cJSON_GetObjectItem(pMessageItem, "topic");

						if (pAt && pPayload && pTopic)
						{
							message.strCreatAt = pAt->valuestring;
							message.strPayload = pPayload->valuestring;
							message.strTopic = pTopic->valuestring;
							m_listMessages.push_back(message);
						}
					}
				}
			}
		}

		cJSON_Delete(pRoot);
		m_strFilePath = sPath;
		return true;
	}

	return false;
}
bool CProduct::Start()
{
	return Start(m_chNode.mqtt.strSvrAdress, m_chNode.mqtt.strUser, m_chNode.mqtt.strPassword);
}
bool CProduct::Start(std::string host, std::string user, std::string password)
{
	if (m_bWork)
		return true;

	m_strHostAddress = host;
	m_strUser = user;
	m_strPassword = password;
	m_bWork = true;
	m_pThread = std::thread(&CProduct::Th_Work, this);
	return true;
}
bool CProduct::End()
{
	if (!m_bWork)
		return true;

	m_bWork = false;
	m_pThread.join();

	return true;
}

int64 CProduct::strTimeToMilliseconds(std::string strTime)
{
	std::string strTimeSub1, strTimeSub2;
	int64 ret = 0;
	size_t offset = strTime.rfind(":");
	if (offset > 0)
	{
		strTimeSub1 = strTime.substr(0, offset);
		strTimeSub2 = strTime.substr(offset + 1);

		XTime t(strTimeSub1);
		ret = t.tv_sec() * 1000 + atoi(strTimeSub2.c_str());
	}
	return ret;
}

int64 CProduct::strTimeToMillisecondsSrt(std::string strTime)
{
	std::string strTimeSub1, strTimeSub2;
	int64 ret = 0;
	size_t offset = strTime.rfind(".");
	if (offset > 0)
	{
		strTimeSub1 = strTime.substr(0, offset);
		strTimeSub2 = strTime.substr(offset + 1);

		XTime t(strTimeSub1);
		ret = t.tv_sec() * 1000 + atoi(strTimeSub2.c_str());
	}
	return ret;
}

void CProduct::Th_Work()
{
	std::list<stuMqMessage> m_listTemp;
	//time_t nLastSendTime = 0;
	//time_t nLastDataTime = 0;
	
	time_t tStart = time(nullptr);
	if (m_listMessages.size() == 0)
	{
		Log(Tool::Warning, "mqtt data is null,send thread exit");
		return;
	}
	try
	{
		mqtt::async_client cli(m_strHostAddress, m_strID.c_str()/*, "./persist"*/);

		auto connOpts = mqtt::connect_options_builder()
			.user_name(m_strUser.c_str())
			.password(m_strPassword.c_str())
			.clean_session()
			/*.will(mqtt::message(strTopic, "will exit", g_nQOS))*/
			.finalize();

		cli.start_consuming();

		mqtt::token_ptr conntok = cli.connect(connOpts);

		if (!conntok->wait_for(3000))
			Log(Tool::Error, "connect mqtt server failed");
		Log(Tool::Info, "connect mqtt server sucess");

		auto rsp = conntok->get_connect_response();
		int64 nFrame = 0;

		TimeWait tWait;
		while (true)
		{
			if (!m_bWork) {
				//退出线程前，先恢复listmessage
				m_listMessages.insert(m_listMessages.begin(), m_listTemp.begin(), m_listTemp.end());
				m_listTemp.clear();
				break;
			}

			if (!cli.is_connected()) {
				Log(Tool::Info, "Reconnecting...");
				try
				{
					cli.reconnect();
				}
				catch (const mqtt::exception& exc)
				{
					cerr << exc << endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
			}

			//接收到视频结束事件
			if (m_videoEndEvent.TryWait(2))
			{
				//数据还原listMessage
				char buf[256] = { 0 };
				time_t tEnd = time(nullptr);
				sprintf(buf, "<<<<<<<<<<<< use time %d, use count %d, no use %d, nFrame %d >>>>>>>>>>>>>>>\n", tEnd - tStart, m_listTemp.size(), m_listMessages.size(), nFrame);
				OutputDebugString(buf);
				Log(Tool::Info, buf);
				tStart = tEnd;
				m_listMessages.insert(m_listMessages.begin(), m_listTemp.begin(), m_listTemp.end());
				m_listTemp.clear();
				//nLastSendTime = 0;
				//nLastDataTime = 0;
				nFrame = 0;
				m_bStartSend = true;
			}

			if (!m_bStartSend)
				continue;

			if (m_listMessages.size() > 0)
			{
				nFrame++;
				stuMqMessage data = m_listMessages.front();
				int64 dataTime = 0;
				if(m_bJson)
					dataTime = strTimeToMilliseconds(data.strCreatAt);
				else
					dataTime = strTimeToMillisecondsSrt(data.strCreatAt);

				tWait.Wait(dataTime);
				m_listMessages.pop_front();
				m_listTemp.push_back(data);
					
				if (nFrame % 25 ) {
					continue;
				}

				//可以发送了
				mqtt::properties props{
					{ mqtt::property::RESPONSE_TOPIC, data.strTopic },
					{ mqtt::property::CORRELATION_DATA, "1" }
				};
				auto pubmsg = mqtt::message_ptr_builder()
					.topic(data.strTopic)
					.payload(data.strPayload)
					.qos(0)
					.properties(props)
					.finalize();

				Log(Tool::Info, "topic %s start publish", data.strTopic.c_str());
				cli.publish(pubmsg)->wait_for(1000);

				CString out;
				out.Format("time: %s\ntopic:%s\n payload:%s\n", data.strCreatAt.c_str(), data.strTopic.c_str(), data.strPayload.c_str());
				OutputDebugString(out.GetBuffer(0));
			}
			else
			{
				m_bStartSend = false;
			}
		}

		cli.disconnect()->wait();
		Log(Tool::Info,"publish thread exit\n");
	}
	catch (const mqtt::exception& exc)
	{
		cerr << exc << endl;
	}
}