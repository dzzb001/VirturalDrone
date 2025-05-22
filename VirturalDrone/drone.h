#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "Device.h"
#include "UdpM.h"
#include "TcpServer.h"
#include "Sip/MsgParser.h"
#include "Tool/XMutex.h"
#include "Tool/XThread.h"

class CDrone : public XThread
{
public:
	CDrone();
	~CDrone();

	//��ʼ��
	bool Init();

	//����ʼ��
	bool Deinit();
	
	//��ӡ�汾��Ϣ
	void print_version();
protected:
	//�̴߳�����
	virtual void Entry();
protected:
	//�߳��˳���־
	bool m_bStop;

	//�߳��Ѿ��˳���־
	bool m_bThreadQuit;

	//����ID�ʹ������ӳ���б�
	typedef std::unordered_map<std::string, CDevice*> DeviceMap;
	DeviceMap m_mapID2Device;

	//�豸�����б�
	std::vector<CDevice*> m_vecDevice;

	//���յ���sip�����б����ʱ���
	std::vector<std::pair<Sip::CMsg*, Sip::CSubMsg*> > m_vecSipCmd;
	XMutex m_LockSipCmd;

	//std::shared_ptr<CUdpM> m_pUdpM;

	//����������
	std::shared_ptr<CMonitor> m_pMonitor;

#if 0
	//<�ͻ���IP:�˿ڣ� <�豸ID��ͨ��ID�� Sip�ỰID>>
	struct ClientAddrNode
	{
		std::string strDeviceID;
		std::string strChID;
		std::string strCallID;
	};
	std::unordered_map<std::string, ClientAddrNode> m_mapClient;
	XMutex m_lockClient;

	struct ClientCmd
	{
		std::weak_ptr<CTcpClientBase> pClient;
		std::string strDeviceID;
		std::string strChID;
		std::string strCallID;
	};
	//TCP�ͻ������������б����ʱ���
	std::vector<ClientCmd> m_vecClientCmd;
	XMutex m_lockClientCmd;
#endif

protected:

	static void UdpDataCB(
		std::shared_ptr<void> pContext,	//��������
		const char *pData,							//���յ�������ָ��
		int nLen,									//���յ��������ֽ���
		const std::string &strFromIP,				//����ԴIP
		short nFromPort								//����Դ�˿�
	);

#if 0
	static bool ClientCB(
		std::shared_ptr<CContextBase> pContext,
		std::shared_ptr<CTcpClientBase> pClient,
		const std::string &strRemoteIP,
		int nRemotePort
	);

	//tcp��������ʱ��Զ�˵�ַ���������ȡ���ط���˿�
	static void AddrCB(
		LPVOID lpContext,							//���룬��������
		const std::string &strDeviceID,				//���룬�豸ID
		const std::string &strCHID,					//���룬ͨ��ID
		const std::string &strCallID,				//���룬�ỰID
		const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
		int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);
#endif

	//����Sip����
	void ProSipCmd();

	//�����û���������
	//void ProUserCmd();

	//����ͻ�����������
	//void ProClientCmd();

	//���ͻ�������
	void CheckChannelClient();
};

