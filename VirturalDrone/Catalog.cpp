#include "stdafx.h"
#include "Catalog.h"
#include "Sip/Counter.h"
#include "UdpM_G.h"

#define Log LogN(4001)

CDeviceInfo::CDeviceInfo()
{
}

CDeviceInfo::~CDeviceInfo()
{
}
bool CDeviceInfo::ResponseOK(Sip::CMsg* pCmd)
{
	std::string strCallId = pCmd->GetHead("Call-ID");
	if (strCallId == m_strCurCallID && Sip::eCmdMessageR == pCmd->m_nType)
	{
		//本设备目录上报的应答
		Log(Tool::Debug, "接收到CDeviceInfo Response OK\n");
		return true;
	}

	return false;
}

void CDeviceInfo::SendReport()
{
	std::string strCallID, strFromTag;

	Sip::CMsgMsg msg;

	if (m_msgSub.Str().size() > 0)
	{
		msg.SetParam(m_strServerId, m_strLocalId, m_strLocalIP, m_nLocalPort,
			CCounter::GetInstanse().Get("DeviceInfoR"), m_msgSub.Str()[0], strCallID, strFromTag);
		msg.Make();
		strCallID = msg.GetHead("Call-ID");
		strFromTag = msg.m_From.strTag;

		m_strCurCallID = strCallID;

		std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
		if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
		{
			Log(Tool::Error, "<%s>发送设备信息查询上报消息失败", "");
		}
		else
		{
			Log(Tool::Info, "<%s>发送设备信息查询上报消息成功", "");
		}
	}
}

CCatalog::CCatalog()
{
	m_bNeedSend = true;
	m_running = false;
}

CCatalog::~CCatalog()
{
}

int g_Count2 = 0;
bool CCatalog::ResponseOK(Sip::CMsg* pCmd)
{
	std::string strCallId = pCmd->GetHead("Call-ID");
	if (strCallId == m_strCurCallID && Sip::eCmdMessageR == pCmd->m_nType)
	{
		//本设备目录上报的应答
		Log(Tool::Debug, "接收到ResponseOK！[%d]\n", g_Count2);
		m_bNeedSend = true;
		return true;
	}

	return false;
}

#define TIMEOUT 5
//目录查询命令的处理线程
void CCatalog::Entry()
{
	m_running = true;
	std::string strCallID, strFromTag;

	nTime = time(NULL);
	for (size_t i = 0; i < m_msgSub.Str().size(); )
	{
		if (m_bNeedSend)
		{
			m_bNeedSend = false;
			if (time(NULL) > nTime + TIMEOUT)
			{
				Log(Tool::Error, "接收目录响应超时\n");
				break;
			}
			Sip::CMsgMsg msg;
			msg.SetParam(m_strServerId, m_strLocalId, m_strLocalIP, m_nLocalPort,
				CCounter::GetInstanse().Get("CatalogR"), m_msgSub.Str()[i], strCallID, strFromTag);
			msg.Make();
			strCallID = msg.GetHead("Call-ID");
			strFromTag = msg.m_From.strTag;

			m_strCurCallID = strCallID;

			std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
			if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
			{
				m_bNeedSend = true;
				Log(Tool::Error, "<%s>发送第<%d>个目录上报消息失败！", "", i);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			else
			{
				//m_bNeedSend = false;
				Log(Tool::Info, "<%s>发送第<%d>个目录上报消息成功 -- 共<%d>！", "", i, ++g_Count2);
				//++g_Count2;
				++i;
			}
			nTime = time(NULL);
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	Log(Tool::Info, "共发送第<%d>个目录上报消息成功！\n", g_Count2);
	Log(Tool::Info, "目录上报线程结束退出\n");
	m_running = false;
}
