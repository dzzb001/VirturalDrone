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

//tcp被动传输时，远端地址输出，并获取本地服务端口
typedef void(*CT_Addr)(
	void * lpContext,							//输入，环境变量
	const std::string &strDeviceID,				//输入，设备ID
	const std::string &strCHID,					//输入，通道ID
	const std::string &strCallID,				//输入，会话ID
	const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
	int nRemotePort							//输入，远端接收Rtp的端口
	);

	CDevice(void);

	~CDevice(void);

	//注册回调函数
	void RegCB(CT_Addr pfn, void * lpContext)
	{
		m_pfnAddr = pfn;
		m_lpContext = lpContext;
	}

	//设置参数
	void SetParam(
		const std::string &strLocalIp,			//本地IP
		int	nLocalPort,							//本地端口
		const std::string &strServerIp,			//服务器的IP
		int	nServerPort,						//账户PORT
		const std::string &strServerID,			//服务器的ID
		const std::string &strID,				//设备ID
		const std::string &strPassword,			//密码
		int nExpire,							//过期时间，单位秒
		int nHeartInterval,						//心跳间隔，单位秒
		int nHeartCount							//失去心跳几次认为断开
		);

	//添加通道，本函数应该在调用SetParam后调用
	void AddCh(
		const std::string &strType,				//通道的类型
		const std::string &strID,				//通道的ID
		const std::string &strName,				//通道名称
		const std::string &strFile,				//视频源pcap文件	
		const std::string &strFileRecord,		//录像url
		const std::string &strPayload,			//负载类型
		const std::string &strLocalIp,			//本地IP
		int	nLocalPort,							//本地端口
		const std::string &strServerIp,			//服务器的IP
		int	nServerPort,						//账户PORT
		const std::string &strServerID			//服务器的ID
		);

	//输入命令
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//用户命令输入
	void UserCmdIn(const std::string &strID,	//通道ID
		const std::string &strType,				//时间类型，比如Alarm
		std::vector<std::string> &vecParam		//参数列表
		);

	//驱动内部处理
	void Process();

	//注销设备，已经注销成功返回true，否则返回false，本函数应该被重复调用，直至返回true
	bool Unregister();

	//输入Tcp客户端
	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strChID, const std::string &strCallID);

	void CheckClient();
	//处理目录上报线程处理函数

protected:

	//注册和维持心跳的对象
	Sip::CRegister m_Register;

	//<ID, 通道对象>列表
	typedef std::unordered_map<std::string, CChannel*> ChannelMap;
	ChannelMap m_mapID2Ch;
	ChannelMap m_mapID2ChRerocd;


	//上报目录命令的CallID
	std::multiset<std::string> m_setCallID;
	XMutex m_LocksetCallID;

	//TCP被动传输视频时，地址回调函数
	CT_Addr m_pfnAddr;
	void * m_lpContext;

	CCatalog  *m_pCatalog;
	CDeviceInfo m_deviceInfo;
protected:

	//设备上下线的处理回调函数
	static void RegisterCB(void * lpContext, bool bRegister);

	//目录查询命令的处理
	void OnCatalogQ(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//上报目录
	void ReportCatalog(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);
	void ReportCatalog2(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//设备信息查询命令的处理
	void OnDeviceInfoQ(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd);
	void ReportDeviceInfo(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd);

	//应答命令
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent="");

	static void AddrCB(
		void * lpContext,							//输入，环境变量
		const std::string &strCHID,					//输入，通道ID
		const std::string &strCallID,				//输入，会话ID
		const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
		int nRemotePort							//输入，远端接收Rtp的端口
	);

};
