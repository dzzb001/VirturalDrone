#pragma once

#include "Sip/MsgParser.h"
#include "Channel.h"
#include "Sip/Register.h"
#include <set>

//#include "net/TcpServer.h"
#include "TcpServer.h"
#include "UdpM.h"

#include "Catalog.h"

class CDevice
{
public:

//tcp��������ʱ��Զ�˵�ַ���������ȡ���ط���˿�
typedef void(*CT_Addr)(
	void * lpContext,							//���룬��������
	const std::string &strDeviceID,				//���룬�豸ID
	const std::string &strCHID,					//���룬ͨ��ID
	const std::string &strCallID,				//���룬�ỰID
	const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
	int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);

	CDevice(void);

	~CDevice(void);

	//ע��ص�����
	void RegCB(CT_Addr pfn, void * lpContext)
	{
		m_pfnAddr = pfn;
		m_lpContext = lpContext;
	}

	//���ò���
	void SetParam(
		const std::string &strLocalIp,			//����IP
		int	nLocalPort,							//���ض˿�
		const std::string &strServerIp,			//��������IP
		int	nServerPort,						//�˻�PORT
		const std::string &strServerID,			//��������ID
		const std::string &strID,				//�豸ID
		const std::string &strPassword,			//����
		int nExpire,							//����ʱ�䣬��λ��
		int nHeartInterval,						//�����������λ��
		int nHeartCount							//ʧȥ����������Ϊ�Ͽ�
		);

	//���ͨ����������Ӧ���ڵ���SetParam�����
	void AddCh(
		const std::string &strType,				//ͨ��������
		const std::string &strID,				//ͨ����ID
		const std::string &strName,				//ͨ������
		const std::string &strFile,				//��ƵԴpcap�ļ�	
		const std::string &strFileRecord,		//¼��url
		const std::string &strPayload,			//��������
		const std::string &strLocalIp,			//����IP
		int	nLocalPort,							//���ض˿�
		const std::string &strServerIp,			//��������IP
		int	nServerPort,						//�˻�PORT
		const std::string &strServerID			//��������ID
		);

	//��������
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//�û���������
	void UserCmdIn(const std::string &strID,	//ͨ��ID
		const std::string &strType,				//ʱ�����ͣ�����Alarm
		std::vector<std::string> &vecParam		//�����б�
		);

	//�����ڲ�����
	void Process();

	//ע���豸���Ѿ�ע���ɹ�����true�����򷵻�false��������Ӧ�ñ��ظ����ã�ֱ������true
	bool Unregister();

	//����Tcp�ͻ���
	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strChID, const std::string &strCallID);

	void CheckClient();
	//����Ŀ¼�ϱ��̴߳�����

protected:

	//ע���ά�������Ķ���
	Sip::CRegister m_Register;

	//<ID, ͨ������>�б�
	typedef std::unordered_map<std::string, CChannel*> ChannelMap;
	ChannelMap m_mapID2Ch;
	ChannelMap m_mapID2ChRerocd;


	//�ϱ�Ŀ¼�����CallID
	std::multiset<std::string> m_setCallID;
	XMutex m_LocksetCallID;

	//TCP����������Ƶʱ����ַ�ص�����
	CT_Addr m_pfnAddr;
	void * m_lpContext;

	CCatalog  *m_pCatalog;
	CDeviceInfo m_deviceInfo;
protected:

	//�豸�����ߵĴ���ص�����
	static void RegisterCB(void * lpContext, bool bRegister);

	//Ŀ¼��ѯ����Ĵ���
	void OnCatalogQ(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//�ϱ�Ŀ¼
	void ReportCatalog(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);
	void ReportCatalog2(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//�豸��Ϣ��ѯ����Ĵ���
	void OnDeviceInfoQ(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd);
	void ReportDeviceInfo(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd);

	//Ӧ������
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent="");

	static void AddrCB(
		void * lpContext,							//���룬��������
		const std::string &strCHID,					//���룬ͨ��ID
		const std::string &strCallID,				//���룬�ỰID
		const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
		int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);

};
