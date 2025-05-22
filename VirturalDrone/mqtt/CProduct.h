#pragma once
#include <string>
#include <thread>
#include <list>
#include <map>
#include <unordered_map>
#include "Config.h"
#include "Tool/XEvent.h"

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

	bool InitMqtt(std::string strId);
	bool DeInitMqtt();
	bool LoadJson(char* sPath);
	bool LoadSrt(char* sPath);
	bool Start();
	bool Start(std::string host, std::string user, std::string password);
	bool End();

	void SetTopic(const char* topic) { m_strTopic = topic; }
	bool ParserFont(std::string strFontContent, std::list<std::unordered_map<std::string, std::string>>& listFont);
	void ChangeSrtToJson();
	void Th_Work();
	long long strTimeToMilliseconds(std::string strTime);
	long long strTimeToMillisecondsSrt(std::string strTime);

	//通知开始发送数据
	void NotifyStartSendData() { m_videoEndEvent.Set(); }
public:
	std::thread       m_pThread;
	bool			  m_bWork;

	std::list<stuMqMessage> m_listMessages;

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
};

