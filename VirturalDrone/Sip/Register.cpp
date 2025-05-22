#include "stdafx.h"
#include "Register.h"
#include <sstream>
#include "Counter.h"
#include "../UdpM_G.h"

//注册、注销命令应答的超时时间，单位豪秒
#define TIMEOUT_REG 60000

//退出时，注销命令应答的超时等间，单位豪秒
#define TIMEOUT_UNREG 5000

//注册时效的最小值，单位秒
#define EXPIRES_MIN 600

//提前多少时间重新注册，单位毫秒
#define REREGISTER_OFFSET 300000

#define Log LogN(6111)

//std::atomic<int> Sip::CRegister::m_nRegisterCount = 1;
std::atomic<int> Sip::CRegister::m_nRegisterCount(1);

namespace Sip
{
CRegister::CRegister(void)
: m_nHeartCount(3)
, m_nHeartInterval(60000)
, m_nExpire(3600000)
, m_nState(eSInit)
, m_dwTime(0)
, m_bAuth(false)
, m_dwRegOK(0)
, m_nFaildCount(0)
, m_pfn(NULL)
, m_lpContext(NULL)
{
}

CRegister::~CRegister(void)
{
}

//注册注册状态改变回调函数
void CRegister::RegCallBack(CT_Register pfn, LPVOID lpContext)
{
	m_pfn = pfn;
	m_lpContext = lpContext;
}

/*******************************************************************************
*功    能:	设置参数
*输入参数:	strID				-- 账户ID
*			strPassword			-- 密码
*			nExpire				-- 过期时间，单位秒
*			nHeartInterval		-- 心跳间隔，单位秒
*			nHeartCount			-- 失去心跳几次认为断开
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-7-28	张斌			创建		
*******************************************************************************/
void CRegister::SetParam(const std::string &strLocalIp, int	nLocalPort,
						 const std::string &strServerIp,int	nServerPort, const std::string &strServerID,
						 const std::string &strID,
						 const std::string &strPassword, int nExpire, int nHeartInterval, 
						 int nHeartCount )
{
	if (nExpire < EXPIRES_MIN)
	{
		Log(Tool::Info, "[%s]注册的有效时间<%d>秒 < %d秒，将被纠正为%d秒！",
			__FUNCTION__, nExpire, EXPIRES_MIN, EXPIRES_MIN);
		nExpire = /*1800*/600;
	}
	m_strLocalIP = strLocalIp;
	m_nLocalPort = nLocalPort;
	m_strServerIP = strServerIp;
	m_nServerPort = nServerPort;
	m_strServerID = strServerID;
	m_strID = strID;
	m_strPassword = strPassword;
	m_nExpire = nExpire*1000;
	m_nHeartInterval = nHeartInterval*1000;
	m_nHeartCount = nHeartCount;
}

//输入命令
bool CRegister::CmdIn(CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	if (m_strCallID != pCmd->GetHead("Call-ID"))
	{
		Log(Tool::Warning, "[%s]CallID不匹配<%s> != <%s>，非 Reg 命令被丢弃",
			__FUNCTION__, m_strCallID.c_str(), pCmd->GetHead("Call-ID").c_str());
		return false;
	}
	switch(pCmd->m_nType)
	{
	case eCmdRegisterR:		OnRegisterR((CMsgRBase*)pCmd, pSubCmd);		break;
	case eCmdMessageR:		OnHeartR((CMsgRBase*)pCmd, pSubCmd);		break;	
	default:
		Log(Tool::Warning, "[%s]收到不期望的命令类型<%d>", __FUNCTION__, pCmd->m_nType);
		break;;
	}
	return true;
}

//处理函数
void CRegister::Process()
{
	switch(m_nState)
	{
	case eSInit:			OnInit();			break;
	case eSRegister:		OnRegister();		break;
	case eSHeart:			OnHeart();			break;
	case eSUnregister:		OnUnRegister();		break;
	default: break;
	}
	if (0 != m_dwRegOK && GetTickCount() - m_dwRegOK > (DWORD)m_nExpire)
	{
		SetRegister(false);	
	}
}

//注销，已注销成功返回true，否则返回false
bool CRegister::UnregisterQ()
{
	if (eSInit == m_nState || 0 == m_dwRegOK)
	{
		return true;
	}
	if (eSUnregister == m_nState)
	{
		if (GetTickCount() - m_dwTime >= TIMEOUT_UNREG)
		{
			//已经发送过注销消息而且超时了
			return true;
		}
		return false;
	}

	//发送注销命令
	Register(0, "", "", m_strRegFromTag, m_strRegCallID);
	return false;
}

bool CRegister::IsRegisted()
{
	return 0 != m_dwRegOK;	
}

//初始状态，直接发注册消息
void CRegister::OnInit()
{
	Register(m_nExpire/1000);
}

//注册状态，检查注册应答有没有超时
void CRegister::OnRegister()
{
	if (GetTickCount() - m_dwTime >= TIMEOUT_REG)
	{
		//超时了，重发
		Log(Tool::Warning, "[%s]第<%d>次等待注册应答超时！", __FUNCTION__, m_nFaildCount);
		Register(m_nExpire/1000);
	}
}

//心跳状态，检查心跳应答有没有超时
void CRegister::OnHeart()
{
	//检查是否到了重新注册的时间了
	if (GetTickCount() - m_dwRegOK > DWORD(m_nExpire - REREGISTER_OFFSET))
	{
		//提前5分钟重新注册
		Log(Tool::Info, "[%s]注册即将到期，重新注册！", __FUNCTION__);
		Register(m_nExpire/1000, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	if (GetTickCount() - m_dwTime < (DWORD)m_nHeartInterval)
	{
		//还没有到心跳间隔时间
		return;
	}
	if (m_nFaildCount != 0)
	{
		Log(Tool::Warning, "[%s]第<%d>次等待心跳应答超时！", __FUNCTION__, m_nFaildCount);
	}
	if (m_nFaildCount >= (DWORD)m_nHeartCount)
	{
		Log(Tool::Warning, "[%s]心跳失败次数<%d>达到阈值<%d>，将注销本次注册！",
			__FUNCTION__, m_nFaildCount, m_nHeartCount);
		SetRegister(false);
		Register(0, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	Heart();	
}

//注销状态
void CRegister::OnUnRegister()
{
	if (GetTickCount() - m_dwTime < TIMEOUT_REG)
	{
		return;
	}
	if (m_nFaildCount < 3)
	{
		//超时了，重发
		Log(Tool::Warning, "[%s]第<%d>次注销命令超时，重发", __FUNCTION__, m_nFaildCount);
		Register(0, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	Log(Tool::Warning, "[%s]注销失败次数<%d>达到3次，将忽略注销重新注册！", __FUNCTION__, m_nFaildCount);
	SetState(eSInit);
}

/*******************************************************************************
*功    能:	收到注册应答的处理
*输入参数:	
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-2	张斌			创建		
*******************************************************************************/
void CRegister::OnRegisterR(CMsgRBase *pCmd, CSubMsg *pSubMsg)
{
	if (eSRegister != m_nState && eSUnregister != m_nState)
	{
		Log(Tool::Warning, "收到不期望的注册应答消息，当前状态<%d>, 将被丢弃", m_nState);
		return;
	}
	m_nFaildCount = 0;
	switch (pCmd->m_FirstLine.nReply)
	{
	case eROK:
		{
			if (eSUnregister == m_nState)
			{
				//注销成功
				SetState(eSInit);
				SetRegister(false);
				m_strRegFromTag = "";
				m_strRegFromTag = "";
			}
			else if(eSRegister == m_nState)
			{
				//注册成功，立刻发送心跳
				SetRegister(true);
				m_strRegCallID = pCmd->GetHead("Call-ID");
				m_strRegFromTag = pCmd->m_From.strTag;
				Heart();
			}
		}		
		break;
	case  eRUnAuthorize: //未授权的注册
		if (!m_bAuth)
		{
			Register(
				eSRegister== m_nState ? m_nExpire/1000 : 0, 
				pCmd->m_Auth.strNonce,
				pCmd->m_Auth.strRealm,
				pCmd->m_From.strTag,
				pCmd->GetHead("Call-ID"),
				pCmd->m_Auth.strOpaque
				);
		}
		else
		{
			Log(Tool::Error, "授权信息错误，注册/注销失败！");
		}
		break;
	default:
		Log(Tool::Warning, "注册应答值<%d>, 未处理！", pCmd->m_FirstLine.nReply);
		break;
	}
}

//收到心跳应答
void CRegister::OnHeartR(CMsgRBase *pCmd, CSubMsg *pSubMsg)
{
	if(eSHeart != m_nState)
	{
		Log(Tool::Warning, "收到不期望的Message消息，当前状态<%d>,Heart 将被丢弃", m_nState);
		return;
	}
	m_nFaildCount = 0;	
	if (Sip::eROK != pCmd->m_FirstLine.nReply)
	{
		Log(Tool::Warning, "心跳应答不是200OK<%d - %s>", pCmd->m_FirstLine.nReply, pCmd->m_FirstLine.strReply.c_str());
		SetState(eSInit);
		SetRegister(false);
		return;
	}
	//Log("收到心跳应答");
}

//设置当前状态
void CRegister::SetState(int nState)
{
	if (nState != m_nState)
	{
		m_nFaildCount = 0;
		m_nState = nState;
	}
	m_dwTime = GetTickCount();
}

//设置注册状态
void CRegister::SetRegister(bool bRegister)
{
	if (bRegister)
	{
		if (0 == m_dwRegOK)
		{
			Log(Tool::Info, "[%s]<%s>上线！ -- 在线数<%d>", __FUNCTION__, m_strID.c_str(), m_nRegisterCount.fetch_add(1));
			if (NULL != m_pfn)
			{
				m_pfn(m_lpContext, true);
			}
		}
		m_dwRegOK = GetTickCount();
	}
	else
	{
		if (0 != m_dwRegOK)
		{
			Log(Tool::Info, "[%s]<%s>下线！ --- 在线数<%d>", __FUNCTION__, m_strID.c_str(), m_nRegisterCount.fetch_sub(1));
			if (NULL != m_pfn)
			{
				m_pfn(m_lpContext, false);
			}
		}
		m_dwRegOK = 0;
	}
}

//发送注册消息
void CRegister::Register( int nExpire, const std::string &strNonce /* = "" */, const std::string &strRealm /* = "" */, 
						 const std::string &strFromTag /* = "" */, const std::string &strCallID /* = "" */,
						 const std::string &strOpaque /* = "" */ )
{
	m_bAuth = !strRealm.empty();
	CMsg::AuthNode auth;
	auth.strNonce = strNonce;
	auth.strRealm = strRealm;
	auth.strOpaque = strOpaque;
	CMsgReg msg;
	msg.SetParma(m_strServerID, m_strID, m_strLocalIP, m_nLocalPort,
		CCounter::GetInstanse().Get(m_strID+"REGISTER"), auth, m_strPassword, nExpire, strFromTag, strCallID);
	msg.Make();
	//if(!Tool::CUdpY::GetInstance().Send(msg.Str().c_str(), msg.Str().size()))
	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), m_strServerIP, m_nServerPort))
	{
		Log(Tool::Error, "发送注册消息失败！");
		return;
	}

	Log(msg.Str().c_str());
	Log("\n");
	SetState(0 != nExpire ? eSRegister : eSUnregister);
	m_strCallID = msg.GetHead("Call-ID");
	m_nFaildCount++;
}

//心跳消息
void CRegister::Heart()
{
	CSubMsgKeepAlive msgSub;
	msgSub.SetParam(CCounter::GetInstanse().Get(m_strID + "SN_HEART"), m_strID, eStatusOK);
	msgSub.Make();
	CMsgMsg msg;
	msg.SetParam(m_strServerID, m_strID, m_strLocalIP, m_nLocalPort,
		CCounter::GetInstanse().Get(m_strID + "HEART"), msgSub.Str()[0]);
	msg.Make();

	//if(!Tool::CUdpY::GetInstance().Send(msg.Str().c_str(), msg.Str().size()))
	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), m_strServerIP, m_nServerPort))
	{
		Log(Tool::Error, "发送心跳消息失败！");
	}
	m_nFaildCount++;
	m_strCallID = msg.GetHead("Call-ID");
	SetState(eSHeart);
}

}
