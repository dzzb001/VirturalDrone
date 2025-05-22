#include "stdafx.h"
#include "Channel.h"
#define Log LogN(4000)

CChannel::CChannel( const std::string &strDevID, const std::string &strID,
				   const std::string &strName )
: m_strDevID(strDevID)
, m_strID(strID)
, m_strName(strName)
{
}

CChannel::~CChannel(void)
{
}

//命令输入
void CChannel::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	Log(Tool::Info, "本通道<%s>不会处理任何Sip命令！", m_strID.c_str());

}

//命令输入
void CChannel::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd, bool &bUsed)
{
	Log(Tool::Info, "本通道<%s>不会处理任何Sip命令！", m_strID.c_str());
}

//用户命令输入
void CChannel::UserCmdIn(const std::string &strType, std::vector<std::string> &vecParam)
{
	Log(Tool::Info, "本通道<%s>不会处理任何用户输入命令！", m_strID.c_str());
}

void CChannel::CheckClient()
{
	Log(Tool::Info, "本通道<%s>不会处理客户端断开检测！", m_strID.c_str());
}

void CChannel::SetParam(std::string strServerId, std::string strServerIp, int nServerPort, std::string strLocalIP, int nLocalPort)
{
	m_strServerID = strServerId;
	m_strServerIP = strServerIp;
	m_nServerPort = nServerPort;
	m_strLocalIP = strLocalIP;
	m_nLocalPort = nLocalPort;
}

