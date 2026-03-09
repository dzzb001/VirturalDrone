#pragma once
#include <string>
#include <thread>
#include <list>
#include <map>
#include <unordered_map>
#include "Config.h"
#include "Tool/XEvent.h"

#include "mqtt/async_client.h"
#include "mqtt/client.h"

typedef struct _stuMqMessage {
	std::string strCreatAt;
	std::string strPayload;
	std::string strTopic;
}stuMqMessage;

class CProduct
{
public:
	CProduct();
	~CProduct();

	bool InitMqtt(std::string strId, bool bFromSrtFile = true);
	bool DeInitMqtt();
	bool LoadJson(char* sPath);
	bool LoadSrt(char* sPath);
	bool Start();
	bool Start(std::string host, std::string user, std::string password);
	bool End();

	void SetTopic(const char* topic) { m_strTopic = topic; }
	bool ParserFont(std::string strFontContent, std::list<std::unordered_map<std::string, std::string>>& listFont);
	void ChangeSrtToJson();

	//读取文件获取无人机数据的工作线程
	void Th_Work();


	long long strTimeToMilliseconds(std::string strTime);
	long long strTimeToMillisecondsSrt(std::string strTime);

	//通知开始发送数据
	void NotifyStartSendData() { m_videoEndEvent.Set(); }

	//读取SEI获取无人机数据的工作线程
	void Th_Work2();
	bool pushMqttData(char* data, unsigned int data_len);
public:
	std::thread       m_pThread;
	bool			  m_bWork;

	std::list<stuMqMessage> m_listMessages;

	//存放ffmpeg中的sei数据
	std::list<stuMqMessage> m_listMessages2;
	std::mutex				m_mutexListMessage2;

	//存放srt数据
	std::list<std::unordered_map<std::string, std::string>> m_listFont;

	std::string		m_strHostAddress;
	std::string     m_strUser;
	std::string     m_strPassword;

	std::string		m_strFilePath;
	std::string     m_strTopic;

	bool			m_bJson;

	//mqtt 相关
	Config::ChNode  m_chNode;
	bool			m_bEnable;
	xbase::XEvent   m_videoEndEvent; //视频播放结束事件

	bool			m_bStartSend;

	std::string     m_strID;
	bool			m_bFromSrtFile; //是否从srt/json 文件中读取无人机姿态数据
};

