#pragma once
#include <unordered_map>
#include <vector>
namespace Config
{

struct MqttInfo {
	std::string strEnable;
	std::string strUser;
	std::string strPassword;
	std::string strSvrAdress;
	std::string strTopic;
	std::string strDataPath;
};
//通道信息节点定义
struct ChNode
{
	std::string strType;
	std::string strName;
	std::string strFile;				//实时视频url
	std::string strRecord;				//录像回放url
	std::string strPayLoad;
	std::string strUrl;					//rtsp url
	MqttInfo	mqtt;
};
typedef std::unordered_map<std::string, ChNode> ChMap;

//设备信息节点定义
struct DevNode
{
	std::string strFactory;
	std::string strPassword;
	std::string strName;
	int nExpire;
	int nHeartInterval;
	int nHeartCount;
	ChMap mapCh;
};

//设备列表定义
typedef std::unordered_map<std::string, DevNode> DevMap;

//ch 列表定义
typedef std::unordered_map<std::string, ChNode> ChlMap;

//事件的参数列表定义
typedef std::vector<std::string> ParamVec;

//事件节点定义
struct EventNode
{
	std::string strType;	//事件类型
	std::string strID;		//发生时间的通道ID
	ParamVec vecParam;		//参数列表
};

//事件列表定义
typedef std::unordered_map<std::string, EventNode> EventMap; 

//配置加载类
class CConfig
{
public:
	CConfig(void);

	~CConfig(void);

	static CConfig & GetInstance(){return s_Config;}

	//加载配置
	bool Load();

	//获取信息
	const std::string &GetServerID(){return m_strServerID;}
	const std::string &GetServerIP(){return m_strServerIP;}
	const int GetServerPort(){return m_nServerPort;}
	const std::string &GetLocalIP(){return m_strLocalIP;}
	int GetLocalPort(){return m_nLocalPort;}

	const std::string& GetDaHuaSerIP() { return m_strDahuaPlatIP; }
	int GetDaHuaSerPort() { return m_nDahuaPlatPort; }

	int GetLocalMeidaPort() { return m_nLocalMediaPort; }
	const DevMap &GetDev(){return m_mapDev;}
	bool GetDevInfo(const std::string &strID, DevNode &node);
	bool GetEvent(const std::string &strName, EventNode &node);

	bool GetChMqttInfo(const std::string strChId, ChNode& node);

	bool PushDevice(){return m_bPushDevice;}

	unsigned int GetKeepDays() { return m_nKeepDays; }
	unsigned int GetLevel() { return m_nLevel; }

protected:

	//服务器信息
	std::string m_strServerID;
	std::string m_strServerIP;
	int m_nServerPort;

	//本地信息
	std::string m_strLocalIP;
	int m_nLocalPort;
	int m_nLocalMediaPort;

	//宁德项目对接无人机服务信息
	std::string m_strDahuaPlatIP;
	int m_nDahuaPlatPort;

	
	//设备列表
	DevMap m_mapDev;

	//设备channal 列表
	ChlMap m_mapCh;

	//时间列表
	EventMap m_mapEvent;

	//是否主动上推设备列表
	bool m_bPushDevice;

	//静态实例
	static CConfig s_Config;

	//日志输出等级
	unsigned int m_nLevel;

	//日志保存期限
	unsigned int m_nKeepDays;
};

}
