#include "stdafx.h"
#include "MsgParser.h"
#include <algorithm>

#define Log LogN(556)

namespace Sip
{

CMsgParser::CMsgParser(void)
{
}

CMsgParser::~CMsgParser(void)
{
}

//解析命令
std::pair<CMsg*, CSubMsg*> CMsgParser::Parser(char *pBuff, int nLen)
{
	CMsg *pMsg = NULL;
	CSubMsg *pSubMsg = NULL;
	std::string strMsg(pBuff, nLen);
	int nCmdType = CMsg::CheckType(strMsg);
	switch(nCmdType)
	{
	case eCmdRegister:	
		pMsg = new CMsgReg;
		break;
	case eCmdMessage:	
		pMsg = new CMsgMsg;
		break;
	case eCmdInvite:
	case eCmdAck:			
	case eCmdBye:		
	case eCmdInfo:
		pMsg = new CMsgBase;
		break;
	case eCmdMessageR:		
	case eCmdRegisterR:		
	case eCmdInviteR:		
	case eCmdByeR:			
		pMsg = new CMsgRBase;
		break;
	default:
		break;
	}
	if (NULL == pMsg)
	{
		Log(Tool::Info, "未能获取到命令类型，丢弃！");
		return std::make_pair(pMsg, pSubMsg);	
	}
	if (pMsg->Parse(strMsg) <= 0)
	{
		Log(Tool::Error, "解析命令出错！");
		delete pMsg;
		pMsg = NULL;
		return std::make_pair(pMsg, pSubMsg);	
	}
	if (pMsg->ContentStr().empty())
	{
		return std::make_pair(pMsg, pSubMsg);	
	}
	std::string strHead = pMsg->GetHead("Content-Type");
	int nSubCmdType = CSubMsg::CheckType(strHead, pMsg->ContentStr());
	switch (nSubCmdType)
	{
	case eSubCmdKeepAlive:	
		pSubMsg = new CSubMsgKeepAlive;	
		break;
	case eSubCmdCatalogQ:
		pSubMsg = new CSubMsgCatalogQ;
		break;
	case eSubCmdCatalogR:	
		pSubMsg = new CSubMsgCatalogR;
		break;
	case eSubCmdSDP:	
		pSubMsg = new CSubMsgSdp;
		break;
	case eSubCmdAlarmR:
		pSubMsg = new CSubMsgAlarmR;
	case eSubCmdRecordInfo:
		pSubMsg = new CSubMsgRecordInfo;
		break;
	case eSubCmdRecordInfoR:
		pSubMsg = new CSubMsgRecordInfoR;
		break;
	case eSubCmdRTSP:
		pSubMsg = new CSubMsgInfo;
		break;
	case eSubCmdDeviceInfoQ:
		pSubMsg = new CSubMsgDeviceInfoQ;
		break;
	case eSubCmdDeviceInfoR:
		pSubMsg = new CSubMsgDeviceInfoR;
		break;
	default:				
		break;
	}
	if (NULL == pSubMsg)
	{
		Log(Tool::Warning, "未能获取到子命令类型");
		return std::make_pair(pMsg, pSubMsg);	
	}
	if(!pSubMsg->Parse(pMsg->ContentStr()))
	{
		Log(Tool::Error, "解析子命令失败！");
		delete pSubMsg;
		pSubMsg = NULL;
	}
	return std::make_pair(pMsg, pSubMsg);
}

}
