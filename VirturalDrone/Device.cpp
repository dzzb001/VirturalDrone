#include "stdafx.h"
#include "Device.h"

#include "Sip/Counter.h"

#ifdef _WIN32
#include "Tool/UdpY.h"
#endif
//#include "ChannelM.h"
//#include "ChannelA.h"
#include "Config.h"
//#include "ChannelR.h"
#include "UdpM_G.h"
//#include "ChannelRtspHistroy.h"
#include "ChannelMp4.h"

#define Log LogN(4001)

CDevice::CDevice(void)
	: m_pfnAddr(nullptr)
	, m_lpContext(nullptr)
{
	m_pCatalog = NULL;
}

CDevice::~CDevice(void)
{
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		delete it->second;
	}

	for (ChannelMap::iterator it = m_mapID2ChRerocd.begin(); it != m_mapID2ChRerocd.end(); ++it)
	{
		delete it->second;
	}
}

//设置参数
void CDevice::SetParam(const std::string &strLocalIp, int	nLocalPort, const std::string &strServerIp, int	nServerPort, const std::string &strServerID, const std::string &strID,
					   const std::string &strPassword, int nExpire, int nHeartInterval, 
					   int nHeartCount )
{
	m_Register.RegCallBack(RegisterCB, this);
	m_Register.SetParam(strLocalIp, nLocalPort, strServerIp, nServerPort, strServerID, strID, strPassword, nExpire, nHeartInterval, nHeartCount);
}

//添加通道
void CDevice::AddCh( const std::string &strType, const std::string &strID, const std::string &strName,
					const std::string &strFile, const std::string &strFileRecord, const std::string &strPayload,
					const std::string &strLocalIp,			//本地IP
					int	nLocalPort,							//本地端口
					const std::string &strServerIp,			//服务器的IP
					int	nServerPort,						//账户PORT
					const std::string &strServerID			//服务器的ID 
)
{
	if (m_mapID2Ch.end() != m_mapID2Ch.find(strID))
	{
		Log(Tool::Error, "添加通道<%s>失败 -- 通道已经存在", strID.c_str());
		return;
	}
	if ("Media" == strType)
	{
		/*CChannelM *pCh = new CChannelM(m_Register.LocalID(), strID, strName, strFile, strPayload);
		pCh->RegCB(AddrCB, this);
		m_mapID2Ch[strID] = pCh;*/
	}
	else if ("Alarm" == strType)
	{
		//CChannelA *pCh = new CChannelA(m_Register.LocalID(), strID, strName);
		//m_mapID2Ch[strID] = pCh;
	}
	else if ("Rtsp" == strType) 
	{
		//CChannelR *pCh = new CChannelR(m_Register.LocalID(), strID, strName, strFile, strPayload);
		//注册回调
		//m_mapID2Ch[strID] = pCh;

		//CChannelRtspHistroy *pChHis = new CChannelRtspHistroy(m_Register.LocalID(), strID, strName, strFileRecord, strPayload);
		//pChHis->SetParam(strServerID, strServerIp, nServerPort, strLocalIp, nLocalPort);
		//录像回放
		//m_mapID2ChRerocd[strID] = pChHis;
	}
	else if ("Mp4" == strType)
	{
		CChannelMp4 *pCh = new CChannelMp4(m_Register.LocalID(), strID, strName, strFile, strPayload);
		//注册回调
		m_mapID2Ch[strID] = pCh;
	}
}

/*******************************************************************************
*功    能:	输入命令
*输入参数:	pCmd			-- sip命令
*			pSubCmd			-- Sip命令消息体中的子命令
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-2	张斌			创建		
*******************************************************************************/
void CDevice::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	Log(Tool::Debug, "CDevice::CmdIn <%s>", pCmd->ContentStr().c_str());

	//Log("!!!!!!!!!m_Register.LocalID():%s\npCmd->m_To.strID:%s\npCmd->m_From.strID:%s\npCmd->m_nType:%d\n", m_Register.LocalID(),pCmd->m_To.strID, pCmd->m_From.strID);
	//Log("!!!!!!!!!m_Register.LocalID():%s\npCmd->m_To.strID:%s\npCmd->m_From.strID:%s\npCmd->m_nType:%d\npSubCmd->m_nType:%d\n", m_Register.LocalID().c_str(), pCmd->m_To.strID.c_str(), pCmd->m_From.strID.c_str(), pCmd->m_nType, pSubCmd ? pSubCmd->m_nType : 11111);

	//本设备相关命令
	if(m_Register.LocalID() == pCmd->m_To.strID || m_Register.LocalID() == pCmd->m_From.strID)
	{
		//Log("dev cmdin step1\n");
		if(Sip::eCmdMessage == pCmd->m_nType && NULL != pSubCmd && Sip::eSubCmdCatalogQ == pSubCmd->m_nType)
		{
			//Log("dev cmdin step2\n");
			OnCatalogQ(pCmd, pSubCmd);
			return;
		}
		else if (Sip::eCmdMessage == pCmd->m_nType && NULL != pSubCmd && Sip::eSubCmdDeviceInfoQ == pSubCmd->m_nType)
		{
			OnDeviceInfoQ(pCmd, pSubCmd);
			return;
		}

		if (m_pCatalog) {
			m_pCatalog->ResponseOK(pCmd);
		}

		if (m_Register.CmdIn(pCmd, pSubCmd))
		{
			return;
		}
	}
	else {
		;// Log("CDevice::CmdIn <123456   %s>", pCmd->ContentStr().c_str());
	}

	//通道相关的命令
	ChannelMap::iterator it = m_mapID2Ch.find(pCmd->m_To.strID);
	if (m_mapID2Ch.end() == it)
	{
		it = m_mapID2Ch.find(pCmd->m_From.strID);
	}
	if (m_mapID2Ch.end() == it)
	{
		if(Sip::eCmdInvite == pCmd->m_nType || Sip::eCmdBye == pCmd->m_nType ||
			Sip::eCmdMessage == pCmd->m_nType)
		{
			Reply(Sip::eRNotExist, pCmd, pSubCmd);
		}
		Log(Tool::Warning, "[%s]未找到合适的处理命令的对象", __FUNCTION__);
		return;
	}
	if (Sip::eCmdInvite == pCmd->m_nType && !m_Register.IsRegisted())
	{
		Log(Tool::Warning, "[%s]设备<%s>当前处于下线状态，不接受视频播放命令！",
			__FUNCTION__, m_Register.LocalID().c_str());
		return;
	}

	//判断是否是录像视频请求
	bool bRecordMessage = false;
	ChannelMap::iterator itReocrd = m_mapID2ChRerocd.find(pCmd->m_To.strID);
	if (m_mapID2ChRerocd.end() != itReocrd)
	{
		itReocrd->second->CmdIn(pCmd, pSubCmd, bRecordMessage);
	}
	//处理实时视频相关消息
	if(!bRecordMessage)
		it->second->CmdIn(pCmd, pSubCmd);	
}

/*******************************************************************************
*功    能:	用户命令输入的处理
*输入参数:	strID			-- 通道ID
*			strType,		-- 时间类型，比如Alarm
*			vecParam		-- 参数列表
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-19	张斌			创建		
*******************************************************************************/
void CDevice::UserCmdIn(const std::string &strID, const std::string &strType, 
						std::vector<std::string> &vecParam )
{
	ChannelMap::iterator it = m_mapID2Ch.find(strID);
	if (m_mapID2Ch.end() == it)
	{
		Log(Tool::Error, "处理用户输入命令失败 - 找不到该通道<%s>", strID.c_str());
		return;
	}
	it->second->UserCmdIn(strType, vecParam);	
}

//驱动内部处理
void CDevice::Process()
{
	m_Register.Process();
}

/*******************************************************************************
*功    能:	注销
*输入参数:	
*输出参数: 	
*返 回 值：	已经注销成功返回true，否则返回false
*其它说明:	本函数应该被重复调用，直至返回成功
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-7-28	张斌			创建		
*******************************************************************************/
bool CDevice::Unregister()
{
	//Log("[%s] ID<%s>", __FUNCTION__, m_Register.LocalID().c_str());
	//关闭所有通道
	bool bClosed = true;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		if(!it->second->Close())
		{
			bClosed = false;
		}
	}
	if (!bClosed)
	{
		return false;
	}

	//注销本设备
	return m_Register.UnregisterQ();
}

void CDevice::ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strChID, const std::string &strCallID)
{
#if 0
	auto it = m_mapID2Ch.find(strChID);
	if (m_mapID2Ch.end() != it)
	{
		auto pCh = dynamic_cast<CChannelM*>(it->second);
		if (nullptr != pCh)
		{
			pCh->ClientIn(pClient, strCallID);
			return;
		}	
	}	
	auto p = pClient.lock();
	if (nullptr != p)
	{
		p->QStop();
		return;
	}
#endif
}

//设备上下线的处理回调函数
void CDevice::RegisterCB(void * lpContext, bool bRegister)
{
	CDevice *pThis = (CDevice*)lpContext;
	Log(Tool::Debug, "[%s] ID<%s>", __FUNCTION__, pThis->m_Register.LocalID().c_str());
	if (!bRegister)
	{
		for (ChannelMap::iterator it = pThis->m_mapID2Ch.begin(); it != pThis->m_mapID2Ch.end(); ++it)
		{
			it->second->Close();
		}
	}
	if (bRegister && Config::CConfig::GetInstance().PushDevice())
	{
		Sip::CMsgMsg msg;
		Sip::CSubMsgCatalogQ subMsg;
		subMsg.m_nSN = 0;
		pThis->ReportCatalog(&msg, &subMsg);
	}
}

#define TIMEOUT 5
int g_Count = 0;
//目录查询命令的处理线程
//目录查询命令的处理
void CDevice::OnCatalogQ(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
	if (NULL == pSubCmd || m_Register.LocalID() != pSubCmd->m_strID)
	{
		Reply(Sip::eRNotExist, pCmd, pSubCmd);
		return;
	}
	//Log("开始处理目录！！！！！");
	Reply(Sip::eROK, pCmd, pSubCmd);

	ReportCatalog2(pCmd, pSubCmd);
}

//设备信息查询命令的处理
void CDevice::OnDeviceInfoQ(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd)
{
	if (NULL == pSubCmd || m_Register.LocalID() != pSubCmd->m_strID)
	{
		Reply(Sip::eRNotExist, pCmd, pSubCmd);
		return;
	}
	Log("设备信息查询！！！！！");
	Reply(Sip::eROK, pCmd, pSubCmd);

	ReportDeviceInfo(pCmd, pSubCmd);
}


void CDevice::ReportCatalog2(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
#if 1
	//处理未完成
	/*if (m_pCatalog && !m_pCatalog->ReportComplete())
		return;*/

	//已完成或者已处理完

	if (m_pCatalog)
	{
		m_pCatalog->Stop();
		delete m_pCatalog;
		m_pCatalog = NULL;
	}

	m_pCatalog = new CCatalog;
	m_pCatalog->SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort());

	Sip::CatalogVec vecCat;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		Sip::CatalogNode node;
		node.strDeviceID = it->second->ID();
		node.strName = it->second->Name();
		node.strParentID = m_Register.LocalID();
		vecCat.push_back(node);
	}

	m_pCatalog->m_msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	m_pCatalog->m_msgSub.Make();

	m_pCatalog->StartReport();
#endif
	return;
}
//上报目录
void CDevice::ReportCatalog(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
	Sip::CatalogVec vecCat;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		Sip::CatalogNode node;
		node.strDeviceID = it->second->ID();
		node.strName = it->second->Name();
		node.strParentID = m_Register.LocalID();
		vecCat.push_back(node);
	}
	Sip::CSubMsgCatalogR msgSub;
	msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	msgSub.Make();
	std::string strCallID, strFromTag;
	//Log("开始上报目录,<%d> 目录总数<%d>", pSubCmd->m_nSN, msgSub.Str().size());
	for (size_t i = 0; i < msgSub.Str().size(); )
	{
		//Log("开始发送<%d>个目录", i);
		Sip::CMsgMsg msg;
		msg.SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort(), 
			CCounter::GetInstanse().Get("CatalogR"), msgSub.Str()[i], strCallID, strFromTag);
		msg.Make();
		strCallID = msg.GetHead("Call-ID");
		strFromTag = msg.m_From.strTag;

		std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
		if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
		{
			Log(Tool::Error, "<%s>发送第<%d>个目录上报消息失败！", pCmd->m_To.strID.c_str(), i);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			Log(Tool::Info, "<%s>发送第<%d>个目录上报消息成功 -- 共<%d>！", pCmd->m_To.strID.c_str(), i, ++g_Count);
			m_setCallID.insert(strCallID);
			++i;
		}
		//Tool::CCommon::m_sleep(50);
	}
}

void CDevice::ReportDeviceInfo(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd)
{
	m_deviceInfo.SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort());

	Sip::CatalogVec vecCat;

	m_deviceInfo.m_msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	m_deviceInfo.m_msgSub.Make();

	m_deviceInfo.SendReport();
}

//应答命令
void CDevice::Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent/* ="" */)
{
	Sip::CMsgRBase msg;
	msg.Reply(nReply, (Sip::CMsgBase*)pCmd, strContent);
	msg.Make();

	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
	{
		Log(Tool::Error, "发送应答失败！");
	}
}

void CDevice::AddrCB( void * lpContext, const std::string &strCHID, const std::string &strCallID, const std::string &strRemoteIP,
	int nRemotePort)
{
	CDevice *pThis = (CDevice*)lpContext;
	if (nullptr != pThis->m_pfnAddr)
	{
		pThis->m_pfnAddr(pThis->m_lpContext, pThis->m_Register.LocalID(), strCHID, strCallID, strRemoteIP, nRemotePort);
	}
}

void CDevice::CheckClient()
{
	ChannelMap::iterator itor = m_mapID2Ch.begin();
	for (; itor != m_mapID2Ch.end(); itor++)
	{
		itor->second->CheckClient();
	}
}
