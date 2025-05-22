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

	//初始化
	bool Init();

	//反初始化
	bool Deinit();
	
	//打印版本信息
	void print_version();
protected:
	//线程处理函数
	virtual void Entry();
protected:
	//线程退出标志
	bool m_bStop;

	//线程已经退出标志
	bool m_bThreadQuit;

	//国标ID和处理对象映射列表
	typedef std::unordered_map<std::string, CDevice*> DeviceMap;
	DeviceMap m_mapID2Device;

	//设备对象列表
	std::vector<CDevice*> m_vecDevice;

	//接收到的sip命令列表及访问保护
	std::vector<std::pair<Sip::CMsg*, Sip::CSubMsg*> > m_vecSipCmd;
	XMutex m_LockSipCmd;

	//std::shared_ptr<CUdpM> m_pUdpM;

	//网络驱动器
	std::shared_ptr<CMonitor> m_pMonitor;

#if 0
	//<客户端IP:端口， <设备ID，通道ID， Sip会话ID>>
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
	//TCP客户端连接命令列表及访问保护
	std::vector<ClientCmd> m_vecClientCmd;
	XMutex m_lockClientCmd;
#endif

protected:

	static void UdpDataCB(
		std::shared_ptr<void> pContext,	//环境变量
		const char *pData,							//接收到的数据指针
		int nLen,									//接收到的数据字节数
		const std::string &strFromIP,				//数据源IP
		short nFromPort								//数据源端口
	);

#if 0
	static bool ClientCB(
		std::shared_ptr<CContextBase> pContext,
		std::shared_ptr<CTcpClientBase> pClient,
		const std::string &strRemoteIP,
		int nRemotePort
	);

	//tcp被动传输时，远端地址输出，并获取本地服务端口
	static void AddrCB(
		LPVOID lpContext,							//输入，环境变量
		const std::string &strDeviceID,				//输入，设备ID
		const std::string &strCHID,					//输入，通道ID
		const std::string &strCallID,				//输入，会话ID
		const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
		int nRemotePort							//输入，远端接收Rtp的端口
	);
#endif

	//处理Sip命令
	void ProSipCmd();

	//处理用户输入命令
	//void ProUserCmd();

	//处理客户端连接命令
	//void ProClientCmd();

	//检测客户端连接
	void CheckChannelClient();
};

