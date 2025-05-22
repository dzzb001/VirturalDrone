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
//ͨ����Ϣ�ڵ㶨��
struct ChNode
{
	std::string strType;
	std::string strName;
	std::string strFile;				//ʵʱ��Ƶurl
	std::string strRecord;				//¼��ط�url
	std::string strPayLoad;
	std::string strUrl;					//rtsp url
	MqttInfo	mqtt;
};
typedef std::unordered_map<std::string, ChNode> ChMap;

//�豸��Ϣ�ڵ㶨��
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

//�豸�б���
typedef std::unordered_map<std::string, DevNode> DevMap;

//ch �б���
typedef std::unordered_map<std::string, ChNode> ChlMap;

//�¼��Ĳ����б���
typedef std::vector<std::string> ParamVec;

//�¼��ڵ㶨��
struct EventNode
{
	std::string strType;	//�¼�����
	std::string strID;		//����ʱ���ͨ��ID
	ParamVec vecParam;		//�����б�
};

//�¼��б���
typedef std::unordered_map<std::string, EventNode> EventMap; 

//���ü�����
class CConfig
{
public:
	CConfig(void);

	~CConfig(void);

	static CConfig & GetInstance(){return s_Config;}

	//��������
	bool Load();

	//��ȡ��Ϣ
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

	//��������Ϣ
	std::string m_strServerID;
	std::string m_strServerIP;
	int m_nServerPort;

	//������Ϣ
	std::string m_strLocalIP;
	int m_nLocalPort;
	int m_nLocalMediaPort;

	//������Ŀ�Խ����˻�������Ϣ
	std::string m_strDahuaPlatIP;
	int m_nDahuaPlatPort;

	
	//�豸�б�
	DevMap m_mapDev;

	//�豸channal �б�
	ChlMap m_mapCh;

	//ʱ���б�
	EventMap m_mapEvent;

	//�Ƿ����������豸�б�
	bool m_bPushDevice;

	//��̬ʵ��
	static CConfig s_Config;

	//��־����ȼ�
	unsigned int m_nLevel;

	//��־��������
	unsigned int m_nKeepDays;
};

}
