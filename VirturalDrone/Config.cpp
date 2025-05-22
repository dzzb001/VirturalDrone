#include "stdafx.h"
#include "Config.h"
#include "Tool/FileEx.h"
#include "Tool/MarkupSTL.h"

#define Log LogN(230)

namespace Config
{

CConfig CConfig::s_Config;

CConfig::CConfig(void)
: m_bPushDevice(false)
, m_nLocalPort(0)
, m_nLocalMediaPort(0)
, m_nServerPort(0)
, m_nLevel(0)
, m_nKeepDays(10)
{
}

CConfig::~CConfig(void)
{
}

//加载配置
bool CConfig::Load()
{
	std::string strFile = Tool::CFileEx::GetExeDirectory();
#ifdef _WIN32
	strFile += "\\Config.xml";
#else
	strFile += "/Config.xml";
#endif
	Log("*******************************************************************************");
	Log("配置文件：<%s>", strFile.c_str());
	CMarkupSTL xml;
	if (!xml.Load(strFile.c_str()) || !xml.FindElem("Root"))
	{
		return false;
	}
	xml.IntoElem();
	if (xml.FindElem("Server"))
	{
		m_strServerIP = xml.GetAttrib("IP");
		m_nServerPort = atoi(xml.GetAttrib("Port").c_str());
		m_strServerID = xml.GetAttrib("ID");
	}
	if (xml.FindElem("Local"))
	{
		m_strLocalIP = xml.GetAttrib("IP");
		m_nLocalPort = atoi(xml.GetAttrib("Port").c_str());
		m_nLocalMediaPort = m_nLocalPort;
		m_bPushDevice = (0 !=atoi(xml.GetAttrib("PushDevice").c_str()));
	}
	if (xml.FindElem("DaHuaPlan"))
	{
		m_strDahuaPlatIP = xml.GetAttrib("IP");
		m_nDahuaPlatPort = atoi(xml.GetAttrib("Port").c_str());
	}

	Log("Sip服务器: ID<%s> - 地址<%s:%d>", m_strServerID.c_str(), m_strServerIP.c_str(), m_nServerPort);
	Log("本地:               地址<%s:%d>", m_strLocalIP.c_str(), m_nLocalPort);
	while(xml.FindElem("Device"))
	{
		DevNode nodeDev;
		std::string strDevID = xml.GetAttrib("ID");
		nodeDev.strPassword = xml.GetAttrib("Password");
		nodeDev.strName = xml.GetAttrib("Name");
		nodeDev.nExpire = atoi(xml.GetAttrib("Expire").c_str());
		nodeDev.nHeartInterval = atoi(xml.GetAttrib("Heart").c_str());
		nodeDev.nHeartCount = atoi(xml.GetAttrib("HeartCount").c_str());
		nodeDev.strFactory = xml.GetAttrib("Factory");
		//Log("* 设备 ID<%s> Password<%s> Expire<%d> HeartInterval<%d> HeartCount<%d>",
		//	strDevID.c_str(), nodeDev.strPassword.c_str(), nodeDev.nExpire, nodeDev.nHeartInterval, nodeDev.nHeartCount);
		if (m_mapDev.end() != m_mapDev.find(strDevID))
		{
			Log("该设备ID已经存在，忽略！");
			continue;
		}
		while(xml.FindChildElem("Channel"))
		{
			ChNode nodeCh;
			std::string strChID = xml.GetChildAttrib("ID");
			nodeCh.strType = xml.GetChildAttrib("Type");
			nodeCh.strName = xml.GetChildAttrib("Name");
			nodeCh.strFile = xml.GetChildAttrib("File");
			nodeCh.strPayLoad = xml.GetChildAttrib("Payload");
			nodeCh.strRecord = xml.GetChildAttrib("Record");
			
			if (xml.IntoElem())
			{
				if (xml.FindChildElem("Mqtt"))
				{
					nodeCh.mqtt.strEnable = xml.GetChildAttrib("Enable");
					nodeCh.mqtt.strUser = xml.GetChildAttrib("User");
					nodeCh.mqtt.strPassword = xml.GetChildAttrib("Password");
					nodeCh.mqtt.strSvrAdress = xml.GetChildAttrib("SvrAdress");
					nodeCh.mqtt.strTopic = xml.GetChildAttrib("Topic");
					nodeCh.mqtt.strDataPath = xml.GetChildAttrib("DataPath");
				}
				xml.OutOfElem();
			}
			m_mapCh[strChID] = nodeCh;
			//Log("*   -- 通道 Type<%s> ID<%s> Name<%s> File<%s> Payload<%s>", nodeCh.strType.c_str(),
			//	strChID.c_str(), nodeCh.strName.c_str(), nodeCh.strFile.c_str(), nodeCh.strPayLoad.c_str());
			if (nodeDev.mapCh.end() != nodeDev.mapCh.find(strChID))
			{
				Log("该通道ID已经存在,忽略！");
				continue;
			}
			nodeDev.mapCh[strChID] = nodeCh;
		}
		m_mapDev[strDevID] = nodeDev;
	}

	if (xml.FindElem("Log"))
	{
		m_nKeepDays = atoi(xml.GetAttrib("KeepDays").c_str());
		std::string strLevel = xml.GetAttrib("Level");

		if (strLevel == "Error")
			m_nLevel = 0;
		else if (strLevel == "Warning")
			m_nLevel = 1;
		else if (strLevel == "Info")
			m_nLevel = 2;
		else if (strLevel == "Debug")
			m_nLevel = 3;
		else
			m_nLevel = 0;
	}

	std::unordered_map<char, std::string> mapType;
	mapType['a'] = "Alarm";
	while(xml.FindElem("Event"))
	{
		std::string strName = xml.GetAttrib("Name");
		if (strName.empty())
		{
			continue;
		}
		std::unordered_map<char, std::string>::iterator it = mapType.find(strName.at(0));
		if (mapType.end() == it)
		{
			continue;
		}
		EventNode node;
		node.strType = it->second;
		node.strID = xml.GetAttrib("ID");
		Log("事件：Name<%s> Type<%s> ID<%s>", strName.c_str(), node.strType.c_str(), node.strID.c_str());
		int nCount = 1;
		while(xml.FindChildElem("Param"))
		{
			node.vecParam.push_back(xml.GetChildData());
			Log("	参数[%d]: %s", nCount++, xml.GetChildData().c_str());
		}
		m_mapEvent[strName] = node;
	}
	Log("*******************************************************************************");
	xml.OutOfElem();
	return true;
}

//根据ID获取设备信息
bool CConfig::GetDevInfo(const std::string &strID, DevNode &node)
{
	DevMap::iterator it = m_mapDev.find(strID);
	if (m_mapDev.end() == it)
	{
		return false;
	}
	node = it->second;
	return true;
}

//根据名称获取时间
bool CConfig::GetEvent(const std::string &strName, EventNode &node)
{
	EventMap::iterator it = m_mapEvent.find(strName);
	if (m_mapEvent.end() == it)
	{
		return false;
	}
	node = it->second;
	return true;
}


bool CConfig::GetChMqttInfo(const std::string strChId, ChNode& node)
{
	ChlMap::iterator it = m_mapCh.find(strChId);
	if (m_mapCh.end() == it)
	{
		return false;
	}
	node = it->second;
	return true;
}

}