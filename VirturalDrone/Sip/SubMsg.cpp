#include "stdafx.h"
#include "Config.h"
#include "SubMsg.h"
#include "../Tool/MarkupSTL.h"
#include "../Tool/strCoding.h"
#include <algorithm>
#include <sstream>
#include <map>
#include <iomanip>

//
////#include <algorithm> // std::transform
//#include <string>
//#include <cctype> // std::toupper
//#include <iostream>
//#include <vector>
//#include <functional>

#ifndef _WIN32
#include <time.h>
#endif // _WIN32

#define Log LogN(557)

namespace Sip
{

CSubMsg::CSubMsg(void)
: m_nType(eSubCmdNone)
, m_nSN(0)
{

}

CSubMsg::~CSubMsg(void)
{
}

/*******************************************************************************
*功    能:	检查命令类型
*输入参数:	strContentType	-- 消息文本的格式
*			strMsg			-- 消息文本
*输出参数: 	
*返 回 值：	消息类型枚举值
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-23	张斌			创建		
*******************************************************************************/
int CSubMsg::CheckType(std::string &strContentType, const std::string &strMsg)
{
#if 1
	std::transform(strContentType.begin(), strContentType.end(), strContentType.begin(), (int(*)(int))tolower);

	if (std::string::npos != strContentType.find("sdp"))
	{
		return eSubCmdSDP;
	}
	if (std::string::npos != strContentType.find("rtsp"))
	{
		return eSubCmdRTSP;
	}
	if (std::string::npos == strContentType.find("xml"))
	{
		return eSubCmdNone;
	}
	CMarkupSTL xml;
	xml.SetDoc(strMsg.c_str());
	if (xml.FindElem())
	{
		std::string strTagName = xml.GetTagName();
		std::string strCmdType;
		if (xml.FindChildElem("CmdType"))
		{
			strCmdType = xml.GetChildData();
		}
		std::transform(strTagName.begin(), strTagName.end(), strTagName.begin(), (int(*)(int))tolower);
		std::transform(strCmdType.begin(), strCmdType.end(), strCmdType.begin(), (int(*)(int))tolower);
		if (0 == strTagName.compare("notify"))
		{
			if (0 == strCmdType.compare("keepalive"))
			{
				return eSubCmdKeepAlive;
			}
			else if (0 == strCmdType.compare("alarm"))
			{
				return eSubCmdAlarmN;
			}
		}
		else if (0 == strTagName.compare("query"))
		{
			if (0 == strCmdType.compare("catalog"))
			{
				return eSubCmdCatalogQ;
			}
			else if (0 == strCmdType.compare("alarm"))
			{
				return eSubCmdAlarmQ;
			}
			else if (0 == strCmdType.compare("recordinfo"))
			{
				return eSubCmdRecordInfo;
			}
			else if (0 == strCmdType.compare("deviceinfo"))
			{
				return eSubCmdDeviceInfoQ;
			}
		}
		else if (0 == strTagName.compare("response"))
		{
			if (0 == strCmdType.compare("catalog"))
			{
				return eSubCmdCatalogR;
			}
			else if (0 == strCmdType.compare("alarm"))
			{
				return eSubCmdAlarmR;
			}
			else if (0 == strCmdType.compare("recordinfo"))
			{
				return eSubCmdRecordInfoR;
			}
		}
		//其他类型.....
	}
#endif
	return eSubCmdNone;
}


//
//心跳
//

CSubMsgKeepAlive::CSubMsgKeepAlive()
{
	m_nType = eSubCmdKeepAlive;

}

CSubMsgKeepAlive::~CSubMsgKeepAlive()
{

}

//设置参数
void CSubMsgKeepAlive::SetParam( int nSN, const std::string &strDeviceID, int nState )
{
	m_nSN = nSN;
	m_strID = strDeviceID;
	m_nStatus = nState;
}

//生成
void CSubMsgKeepAlive::Make()
{
	CMarkupSTL xml;
	xml.SetDoc("<?xml version=\"1.0\"?>\n");
	xml.AddElem("Notify");
	xml.AddChildElem("CmdType", "Keepalive");
	std::ostringstream oss;
	oss << m_nSN;
	xml.AddChildElem("SN", oss.str().c_str());
	xml.AddChildElem("DeviceID", m_strID.c_str());
	std::unordered_map<int, std::string> mapStatus;
	mapStatus[eStatusOK] = "OK";
	xml.AddChildElem("Status", mapStatus[m_nStatus].c_str());
	m_vecMsgStr.push_back(xml.GetDoc());
}

CSubMsgCatalogQ::CSubMsgCatalogQ()
{
	m_nType = eSubCmdCatalogQ;
}

CSubMsgCatalogQ::~CSubMsgCatalogQ()
{

}

//目录查询命令的解析
bool CSubMsgCatalogQ::Parse(const std::string &strMsg)
{
	CMarkupSTL xml;
	xml.SetDoc(strMsg.c_str());
	if (!xml.FindElem("Query"))
	{
		return false;
	}
	xml.IntoElem();
	if (!xml.FindElem("CmdType"))
	{
		return false;
	}
	if (0 != xml.GetData().compare("Catalog"))
	{
		return false;
	}
	if (!xml.FindElem("SN"))
	{
		return true;
	}
	m_nSN = atoi(xml.GetData().c_str());
	if (!xml.FindElem("DeviceID"))
	{
		return false;
	}
	m_strID = xml.GetData();
	return true;
}

CSubMsgCatalogR::CSubMsgCatalogR()
{
	m_nType = eSubCmdCatalogR;
}

CSubMsgCatalogR::~CSubMsgCatalogR()
{

}

//设置参数
void CSubMsgCatalogR::SetParam( int nSN, const std::string strDeviceID, const CatalogVec &vecCatalog )
{
	m_nSN = nSN;
	m_strID = strDeviceID;
	m_vecCatalog = vecCatalog;
}

void CSubMsgCatalogR::Make()
{
	Log(Tool::Info, "开始生成目录,目录总数<%d>", m_vecCatalog.size());
	const int nReprotMax = 1;
	CMarkupSTL xml;	
	for (size_t i = 0; i < m_vecCatalog.size(); ++i)
	{
		if (i % nReprotMax == 0)
		{
			if (i != 0)
			{
				m_vecMsgStr.push_back(xml.GetDoc());
			}
			xml.SetDoc("<?xml version=\"1.0\"?>\n");
			xml.AddElem("Response");
			xml.IntoElem();
			xml.AddElem("CmdType", "Catalog");
			xml.AddElem("SN", m_nSN);
			xml.AddElem("DeviceID", m_strID.c_str());
			xml.AddElem("SumNum", m_vecCatalog.size());
			xml.AddElem("DeviceList");
			xml.AddAttrib("Num", m_vecCatalog.size()-i > nReprotMax ? nReprotMax : m_vecCatalog.size()-i);
			xml.IntoElem();
		}
		CatalogNode &node = m_vecCatalog[i];
		xml.AddElem("Item");
		xml.AddChildElem("DeviceID", node.strDeviceID.c_str());
		xml.AddChildElem("Name", node.strName.c_str());
		xml.AddChildElem("Manufacturer", node.strManufacturer.c_str());
		xml.AddChildElem("Model", node.strModel.c_str());
		xml.AddChildElem("Owner", node.strOwner.c_str());
		xml.AddChildElem("CivilCode", node.strCivilCode.c_str());
		xml.AddChildElem("Address", node.strAddress.c_str());
		xml.AddChildElem("Parental", node.nParental);
		xml.AddChildElem("ParentID", node.strParentID.c_str());
		xml.AddChildElem("SafetyWay", node.nSafetyWay);
		xml.AddChildElem("RegisterWay", node.nRegisterWay);
		xml.AddChildElem("Secrecy", node.nSecrecy);
		xml.AddChildElem("Status", node.strStatus.c_str());
	}
#if 0 
	strCoding coding;
	std::string strOut; 
	coding.GB2312ToUTF_8(strOut, (char *)xml.GetDoc().c_str(), xml.GetDoc().size());
	m_vecMsgStr.push_back(strOut); //转换成UTF_8格式。
#else
	m_vecMsgStr.push_back(xml.GetDoc());
#endif
}

CSubMsgSdp::CSubMsgSdp()
{
	m_nType = eSubCmdSDP;
	m_mapParseFun["o"] = ParseO;
	m_mapParseFun["c"] = ParseC;
	m_mapParseFun["m"] = ParseM;
	m_mapParseFun["a"] = ParseA;
	m_mapParseFun["s"] = ParseS;
	m_mapParseFun["y"] = ParseY;
	m_mapParseFun["t"] = ParseT;
	
	m_nRtpPort = 0;
	m_nPSPT = 0;
}

CSubMsgSdp::~CSubMsgSdp()
{
}

/*******************************************************************************
*功    能:	设置参数
*输入参数:	strID			-- 设备ID
*			strIP			-- 地址
*			strPassword		-- 密码
*			strType			-- recvonly、sendonly
*			nRtpPort		-- 发送或者接收Rtp端口
*			strPlayType		-- Play：实时点播 Playback：历史回放 DownLoad：文件下载
*			strSSRC			-- ssrc
*			strPT			-- PT类型，如H264、PS
*			nPSPT			-- PT值
*输出参数: 	
*返 回 值：	
*其它说明:	
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-3	张斌			创建		
*******************************************************************************/
void CSubMsgSdp::SetParam( const std::string &strID, const std::string &strIP,  const std::string &strPassword,
	const std::string &strType, const std::string &strProtocol, const std::string &strMode, int nRtpPort, 
	const std::string &strPlayType, const std::string &strSSRC, const std::string &strPT, int nPT)
{
	m_strID = strID;
	m_strIP = strIP;
	m_strPassword = strPassword;
	m_strType = strType;
	m_strProtocol = strProtocol;
	m_strMode = strMode;
	m_nRtpPort = nRtpPort;
	m_strPlayType = strPlayType;
	m_strSSRC = strSSRC;
	m_mapRtp[nPT] = strPT;
}

void CSubMsgSdp::Make()
{
	std::ostringstream oss;
	oss << "v=0\r\n";
	oss << "o=" << m_strID << " 0 0 IN IP4 " << m_strIP << "\r\n";
	oss << "s=IPC\r\n";
	oss << "c=IN IP4 " << m_strIP << "\r\n";
	oss << "t=0 0\r\n";

	if (m_mapRtp.size() > 0)
	{
		oss << "m=video " << m_nRtpPort << " " << m_strProtocol << " " << m_mapRtp.begin()->first << "\r\n";
	}
	oss << "a=" << m_strType << "\r\n";
	for (std::unordered_map<int, std::string>::iterator it = m_mapRtp.begin(); it != m_mapRtp.end(); ++it)
	{
		oss << "a=rtpmap:" << it->first << " " << it->second << "/90000\r\n";
	}
	if (!m_strMode.empty())
	{
		oss << "a=setup:TCP " << m_strMode << "\r\n";
		oss << "a=connection:new" << "\r\n";
	}
	oss << "a=username:" << m_strID << "\r\n";
	oss << "a=password:" << m_strPassword << "\r\n";
	oss << "y=" << m_strSSRC << "\r\n";
	oss << "f=v/2////a///" << "\r\n";
	m_vecMsgStr.push_back(oss.str());
}


bool CSubMsgSdp::Parse(const std::string &strMsg)
{
	char buff[512];
	std::stringstream ss(strMsg);
	bool bFirst = true;
	while(ss.getline(buff, 512))
	{
		std::string strLine = buff;
		strLine.erase(0, strLine.find_first_not_of(" \t\r\n"));
		strLine.erase(strLine.find_last_not_of(" \t\r\n")+1);
		if (strLine.empty())
		{
			continue;
		}
		std::string::size_type nStart = strLine.find("=");
		if (std::string::npos == nStart)
		{
			continue;
		}
		m_vecLine.push_back(std::make_pair(
			strLine.substr(0, nStart),
			strLine.substr(nStart+1, strLine.size()-nStart)
			));
	}
	for (size_t i = 0; i < m_vecLine.size(); ++i)
	{
		std::pair<std::string, std::string> &node = m_vecLine[i];
		Log(Tool::Info, "%s -- %s", node.first.c_str(), node.second.c_str());
		std::unordered_map<std::string, CT_Parse>::iterator it = m_mapParseFun.find(node.first);
		if (m_mapParseFun.end() == it)
		{
			continue;
		}
		it->second(this, node.first, node.second);
	}
	return true;
}

void CSubMsgSdp::ParseO(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	std::string::size_type nStart = strContent.find(" ");
	if (std::string::npos != nStart)
	{
		pThis->m_strID = strContent.substr(0, nStart);
	}
	pThis->m_strIP = GetContent(strContent, "IP4", " ", " ");
}

void CSubMsgSdp::ParseC(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	pThis->m_strIP = GetContent(strContent, "IP4", " ", " ");
}

void CSubMsgSdp::ParseM(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	if (std::string::npos == strContent.find("video"))
	{
		return;
	}
	if (std::string::npos != strContent.find("TCP/RTP/AVP"))
	{
		pThis->m_strProtocol = "TCP/RTP/AVP";
	}
	else if(std::string::npos != strContent.find("RTP/AVP"))
	{
		pThis->m_strProtocol = "RTP/AVP";
	}
	if (pThis->m_strProtocol.empty())
	{
		return;
	}
	pThis->m_nRtpPort = atoi(GetContent(strContent, "video", " ", " ").c_str());
}

void CSubMsgSdp::ParseA(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	if (std::string::npos != strContent.find("rtpmap"))
	{
		std::string strNo = GetContent(strContent, "rtpmap", ":", " ");
		if (!strNo.empty())
		{
			int nPT = atoi(strNo.c_str());
			std::string strPT = GetContent(strContent, strNo, " ", " ");
			pThis->m_mapRtp[nPT] = strPT;
			if (std::string::npos != strPT.find("PS"))
			{
				pThis->m_nPSPT = nPT;
			}
			else if (std::string::npos != strPT.find("H264"))
			{
				pThis->m_nH264PT = nPT;
			}
		}
	}
	else if (std::string::npos != strContent.find("password"))
	{
		pThis->m_strPassword = GetContent(strContent, "password", ":", " ");
	}
	else if(std::string::npos != strContent.find("setup"))
	{
		pThis->m_strMode = GetContent(strContent, "setup", ":", " ");		 
	}
	else if(std::string::npos != strContent.find("connection"))
	{
		pThis->m_strConnection = GetContent(strContent, "connection", ":", " ");
	}
	else 
	{
		if (0 == strContent.compare("sendonly") || 0 == strContent.compare("recvonly"))
		{
			pThis->m_strType = strContent;
		}
	}
}

void CSubMsgSdp::ParseY(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	pThis->m_strSSRC = strContent;
}

void CSubMsgSdp::ParseS(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	Log(Tool::Debug, "CSubMsgSdp::ParseS ------------<%s>", strContent.c_str());
	if (strContent == "Play" ||strContent=="Playback" || strContent == "Download" )
	{
		Log(Tool::Debug, "CSubMsgSdp::ParseS ------------%s", strContent.c_str());
		pThis->m_strPlayType = strContent;
		Log(Tool::Debug, "CSubMsgSdp::m_strPlayType ------------%s", strContent.c_str());
	}
	else
	{
		Log(Tool::Debug, "==== not paly ..");
	}
}

void CSubMsgSdp::ParseT(CSubMsgSdp *pThis, const std::string &strHead, const std::string &strContent)
{
	Log(Tool::Debug, "CSubMsgSdp::ParseT ------------<%s>", strContent.c_str());
	if (std::string::npos != strContent.find(" "))
	{
		size_t pos = strContent.find(" ");
		pThis->m_strStartTime = strContent.substr(0, pos);
		pThis->m_strEndTime = strContent.substr(pos + 1);
	}
	else {
		pThis->m_strStartTime = strContent;
		pThis->m_strEndTime = "";
	}
}

//获取头域的内容子字符串
std::string CSubMsgSdp::GetContent( const std::string &strMsg, std::string strName, const std::string &strPrefix, 
							 const std::string &strSuffix, const std::string &strSuffixSubby /* = "" */ )
{
	strName.append(strPrefix);
	std::string::size_type nStart = strMsg.find(strName);
	if (std::string::npos == nStart)
	{
		return std::string();		
	}
	nStart += strName.size();
	std::string::size_type nEnd = strMsg.find(strSuffix, nStart);
	if (std::string::npos == nEnd && !strSuffixSubby.empty())
	{		
		nEnd = strMsg.find(strSuffixSubby, nStart);		
	}
	if (std::string::npos == nEnd)
	{
		std::string strEnd = "\r\n";
		nEnd = strMsg.find(strEnd, nStart);
	}
	if (std::string::npos == nEnd)
	{
		nEnd = strMsg.size();
	}
	std::string strRet = strMsg.substr(nStart, nEnd-nStart);
	strRet.erase(0, strRet.find_first_not_of(" "));
	strRet.erase(strRet.find_last_not_of(" ") + 1);
	return strRet;
}

CSubMsgCatalogN::CSubMsgCatalogN()
{
	m_nType = eSubCmdAlarmN;
}

CSubMsgCatalogN::~CSubMsgCatalogN()
{

}

//设置参数
void CSubMsgCatalogN::SetParam( const std::string &strID, int nSN, const AlarmNode &node )
{
	m_strID = strID;
	m_nSN = nSN;
	m_Alarm = node;
	if (m_Alarm.strTime.empty())
	{
#ifdef _WIN32
		time_t t;
		tm lt;
		time(&t);
		localtime_s(&lt, &t);
		localtime_s(&lt, &t);
		std::ostringstream oss;
		oss << lt.tm_year+1900;
		oss << "-" << std::setfill('0') << std::setw(2) << lt.tm_mon+1;
		oss << "-" << std::setfill('0') << std::setw(2) << lt.tm_mday;
		oss << "T" << std::setfill('0') << std::setw(2) << lt.tm_hour;
		oss << ":" << std::setfill('0') << std::setw(2) << lt.tm_min;
		oss << ":" << std::setfill('0') << std::setw(2) << lt.tm_sec;
		m_Alarm.strTime = oss.str();
#else
		time_t t;
		struct tm *lt;
		lt = gmtime(&t);

		std::ostringstream oss;
		oss << lt->tm_year + 1900;
		oss << "-" << std::setfill('0') << std::setw(2) << lt->tm_mon + 1;
		oss << "-" << std::setfill('0') << std::setw(2) << lt->tm_mday;
		oss << "T" << std::setfill('0') << std::setw(2) << lt->tm_hour;
		oss << ":" << std::setfill('0') << std::setw(2) << lt->tm_min;
		oss << ":" << std::setfill('0') << std::setw(2) << lt->tm_sec;
		m_Alarm.strTime = oss.str();
#endif // _WIN32

	}	
}

//生成
void CSubMsgCatalogN::Make()
{
	CMarkupSTL xml;
	xml.SetDoc("<?xml version=\"1.0\"?>\n");
	xml.AddElem("Notify");
	xml.IntoElem();
	xml.AddElem("CmdType", "Alarm");
	xml.AddElem("SN", m_nSN);
	xml.AddElem("DeviceID", m_strID.c_str());
	xml.AddElem("AlarmPriority", m_Alarm.strPriority.c_str());
	xml.AddElem("AlarmMethod", m_Alarm.strMethod.c_str());
	xml.AddElem("AlarmTime", m_Alarm.strTime.c_str());
	if (!m_Alarm.strDescript.empty())
	{
		xml.AddElem("AlarmDescription", m_Alarm.strDescript.c_str());
	}
	if (!m_Alarm.strLonitude.empty())
	{
		xml.AddElem("Longitude", m_Alarm.strLonitude.c_str());
	}
	if (!m_Alarm.strLatitude.empty())
	{
		xml.AddElem("Latitude", m_Alarm.strLonitude.c_str());
	}
	for (size_t i = 0; i < m_Alarm.vecInfo.size(); ++i)
	{
		xml.AddElem("Info", m_Alarm.vecInfo[i].c_str());
	}
	m_vecMsgStr.push_back(xml.GetDoc());
}


CSubMsgAlarmR::CSubMsgAlarmR()
{
	m_nType = eSubCmdAlarmR;
}

CSubMsgAlarmR::~CSubMsgAlarmR()
{

}

//解析
bool CSubMsgAlarmR::Parse(const std::string &strMsg)
{
	CMarkupSTL xml;
	xml.SetDoc(strMsg.c_str());
	if (!xml.FindElem("Response"))
	{
		Log(Tool::Error, "[%s]找不到Response节点！", __FUNCTION__);
		return false;
	}
	xml.IntoElem();
	if (!xml.FindElem("CmdType"))
	{
		Log(Tool::Error, "[%s]找不到CmdType节点！", __FUNCTION__);
		return false;
	}
	if (xml.GetData() != "Alarm")
	{
		Log(Tool::Error, "[%s]CmdType值<%d> = Alarm！", __FUNCTION__, xml.GetData().c_str());
		return false;
	}
	if (xml.FindElem("SN"))
	{
		m_nSN = atoi(xml.GetData().c_str());
	}
	if (xml.FindElem("DeviceID"))
	{
		m_strID = xml.GetData();
	}
	if (xml.FindElem("Result"))
	{
		m_strResult = xml.GetData();
	}
	xml.OutOfElem();
	return true;
}

bool CSubMsgRecordInfo::Parse(const std::string &strMsg)
{
	CMarkupSTL xml;
	xml.SetDoc(strMsg.c_str());
	if (!xml.FindElem("Query"))
	{
		return false;
	}
	xml.IntoElem();
	if (!xml.FindElem("CmdType"))
	{
		return false;
	}
	if (!xml.FindElem("SN"))
	{
		return true;
	}
	m_nSN = atoi(xml.GetData().c_str());
	if (!xml.FindElem("DeviceID"))
	{
		return false;
	}
	m_strID = xml.GetData();

	if (!xml.FindElem("StartTime"))
	{
		return false;
	}
	m_strStartTime = xml.GetData();

	if (!xml.FindElem("EndTime"))
	{
		return false;
	}
	m_strEndTime = xml.GetData();

	if (!xml.FindElem("Type"))
	{
		return false;
	}
	m_strType = xml.GetData();
	return true;
}

bool CSubMsgRecordInfoR::Parse(const std::string &strMsg)
{
	return true;
}

bool CSubMsgInfo::Parse(const std::string &strMsg)
{
	char buff[512];
	std::stringstream ss(strMsg);
	bool bFirst = true;

	while (ss.getline(buff, 512))
	{
		std::string strLine = buff;
		strLine.erase(0, strLine.find_first_not_of(" \t\r\n"));
		strLine.erase(strLine.find_last_not_of(" \t\r\n") + 1);
		if (strLine.empty())
		{
			continue;
		}

		if (bFirst) 
		{
			std::string::size_type nStart = strLine.find(" ");
			if (std::string::npos == nStart)
			{
				continue;
			}
			m_strMode = strLine.substr(0, nStart);
			bFirst = false;
			continue;
		}

		std::string::size_type nStart = strLine.find(":");
		if (std::string::npos == nStart)
		{
			continue;
		}

		std::string strMethod = strLine.substr(0, nStart);

		if (!strMethod.compare("CSeq"))
		{
			m_strSeq = strLine.substr(nStart + 2, strLine.size() - nStart).c_str();
		}
		else if (!strMethod.compare("Scale"))
		{
			m_strScale = strLine.substr(nStart + 2, strLine.size() - nStart).c_str();
		}
	}
	return true;
}

void CSubMsgInfo::Make()
{

}

CSubMsgDeviceInfoQ::CSubMsgDeviceInfoQ()
{
	m_nType = eSubCmdDeviceInfoQ;
}

CSubMsgDeviceInfoQ::~CSubMsgDeviceInfoQ()
{

}

//目录查询命令的解析
bool CSubMsgDeviceInfoQ::Parse(const std::string& strMsg)
{
	CMarkupSTL xml;
	xml.SetDoc(strMsg.c_str());
	if (!xml.FindElem("Query"))
	{
		return false;
	}
	xml.IntoElem();
	if (!xml.FindElem("CmdType"))
	{
		return false;
	}
	if (0 != xml.GetData().compare("DeviceInfo"))
	{
		return false;
	}
	if (!xml.FindElem("SN"))
	{
		return true;
	}
	m_nSN = atoi(xml.GetData().c_str());
	if (!xml.FindElem("DeviceID"))
	{
		return false;
	}
	m_strID = xml.GetData();
	return true;
}

CSubMsgDeviceInfoR::CSubMsgDeviceInfoR()
{
	m_nType = eSubCmdCatalogR;
}

CSubMsgDeviceInfoR::~CSubMsgDeviceInfoR()
{

}

//设置参数
void CSubMsgDeviceInfoR::SetParam(int nSN, const std::string strDeviceID, const CatalogVec& vecCatalog)
{
	m_nSN = nSN;
	m_strID = strDeviceID;
	m_vecCatalog = vecCatalog;
}

void CSubMsgDeviceInfoR::Make()
{
	Log(Tool::Info, "开始生成设备信息");
	CMarkupSTL xml;
	Config::CConfig& config = Config::CConfig::GetInstance();
	Config::DevNode nodeDev;
	if (!config.GetDevInfo(m_strID, nodeDev))
	{
		return;
	}
	xml.SetDoc("<?xml version=\"1.0\"?>\n");
	xml.AddElem("Response");
	xml.IntoElem();
	xml.AddElem("CmdType", "DeviceInfo");
	xml.AddElem("SN", m_nSN);
	xml.AddElem("DeviceID", m_strID.c_str());
	xml.AddElem("DeviceName", nodeDev.strName.c_str());
	xml.AddElem("Result", "OK");
	xml.AddElem("DeviceType", "");
	xml.AddElem("Manufacturer", "Syz");
	xml.AddElem("Model", "");
	xml.AddElem("Firmware", "");
	xml.AddElem("MaxCamera", 1);
	xml.AddElem("MaxAlarm", 0);
	xml.AddElem("Channel", 1);
#if 0 
	strCoding coding;
	std::string strOut;
	coding.GB2312ToUTF_8(strOut, (char*)xml.GetDoc().c_str(), xml.GetDoc().size());
	m_vecMsgStr.push_back(strOut); //转换成UTF_8格式。
#else
	m_vecMsgStr.push_back(xml.GetDoc());
#endif
}

}
