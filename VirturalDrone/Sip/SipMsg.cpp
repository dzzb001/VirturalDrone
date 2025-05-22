#include "stdafx.h"
#include "MsgParser.h"
#include <sstream>
#include <algorithm>
#include <iomanip>
#include "../Tool/Encrypt.h"

#ifndef _WIN32
#include <uuid/uuid.h>
#endif // !_WIN32


#define Log LogN(555)

//���λ��
#define DOMAIN_COUNT 10


namespace Sip
{

CMsg::CMsg()
: m_nType(eCmdNone)
, m_nSrcPort(0)
{	
}

CMsg::~CMsg()
{

}


//����һ���ַ�������Ϣ����
int CMsg::CheckType(const std::string &strMsg)
{
	if (strMsg.empty())
	{
		return eCmdNone;
	}
	//��CSeq��ȡ��Ϣ����
	std::string strCSeq = GetContent(strMsg, "CSeq", ":", "\r\n");
	std::string::size_type nStart = strCSeq.find(" ");
	if (std::string::npos == nStart)
	{
		return eCmdNone;
	}
	nStart+=1;
	std::string strCmd = strCSeq.substr(nStart, strCSeq.size()-nStart);
	std::transform(strCmd.begin(), strCmd.end(), strCmd.begin(), toupper);
	nStart = strMsg.find_first_not_of(" \t\r\n");
	int nEnd = strMsg.find(" ", nStart);
	if (std::string::npos == nEnd)
	{
		return eCmdNone;
	}
	std::string strFirstLineCmd = strMsg.substr(nStart, nEnd-nStart);
	return Str2Type(strFirstLineCmd != strCmd, strCmd);	
}

//��������
int CMsg::Parse(const std::string &strMsg)
{
	//��ȡ�����
	int nLen = GetMsgLen(strMsg);
	if (nLen <= 0)
	{
		return 0;
	}
	//������
	Parse2Line(strMsg);

	//��������
	ParseFirstLine(m_strFirstLine);
	AddParseFun();
	for (size_t i = 0; i < m_vecHead.size(); ++i)
	{
		std::string strHead = m_vecHead[i].first;
		std::transform(strHead.begin(), strHead.end(), strHead.begin(), tolower);
		std::unordered_map<std::string, CT_Parse>::iterator it = m_mapParseFun.find(strHead);
		if(m_mapParseFun.end() == it)
		{
			//Log("ͷ���Ҳ������ʵĽ���������<%s> - <%s>", strHead.c_str(), m_vecHead[i].second.c_str());
			continue;
		}
		it->second(this, strHead, m_vecHead[i].second);
	}

	//��ȡ��������
	m_nType = GetCmdType();

	//����
	int nContentLength = atoi(GetHead(STR_HEAD_CONTENT_LENGTH).c_str());
	if (nContentLength > 0)
	{
		m_strContent = strMsg.substr(strMsg.size()- nContentLength, nContentLength);
	}
	return nLen;
}


//��ȡָ��ͷ���ֵ
std::string CMsg::GetHead(std::string strHead)
{
	std::transform(strHead.begin(), strHead.end(), strHead.begin(), tolower);
	for (size_t i = 0; i < m_vecHead.size(); ++i)
	{
		std::string strNodeHead = m_vecHead[i].first;
		std::transform(strNodeHead.begin(), strNodeHead.end(), strNodeHead.begin(), tolower);
		if ( strNodeHead == strHead)
		{
			return m_vecHead[i].second;			
		}
	}
	return std::string();	
}

void CMsg::Make()
{
	m_strMsg.clear();

	//���ø����ֶ�
	if (m_strFirstLine.empty())
	{
		MakeFirstLine();
	}
	m_strMsg.append(m_strFirstLine);
	m_strMsg.append("\r\n");
	
	AddMakeFun();
	for (size_t i = 0; i < m_vecHead.size(); ++i)
	{
		HeadNode &node = m_vecHead[i];
		if (node.first.empty())
		{
			//Ӧ�ò��ᷢ��
			continue;
		}
		if (node.second.empty())
		{
			MakeHeadText(node.first, node.second);
		}
		if (0 == node.first.compare(STR_HEAD_CONTENT_LENGTH))
		{
			std::ostringstream oss;
			oss << m_strContent.size();
			node.second = oss.str();
		}
		m_strMsg.append(node.first);
		m_strMsg.append(": ");
		m_strMsg.append(node.second);
		m_strMsg.append("\r\n");
	}
	m_strMsg.append("\r\n");

	if (!m_strContent.empty())
	{
		m_strMsg.append(m_strContent);		
	}
}


//��ӽ�������
void CMsg::AddParseFun()
{
	m_mapParseFun["via"] = ParseVia;
	m_mapParseFun["from"] = ParseFromTo;
	m_mapParseFun["to"] = ParseFromTo;
	m_mapParseFun["contact"] = ParseContact;
	m_mapParseFun["cseq"] = ParseCSeq;
	m_mapParseFun["www-authenticate"] = ParseAuth;
	m_mapParseFun["authorization"] = ParseAuth;
	m_mapParseFun["sbuject"] = ParseSubject;
}

//�����װ����
void CMsg::AddMakeFun()
{
	m_mapMakeFun["via"] = MakeVia;
	m_mapMakeFun["from"] = MakeFromTo;
	m_mapMakeFun["to"] = MakeFromTo;
	m_mapMakeFun["contact"] = MakeContact;
	m_mapMakeFun["cseq"] = MakeCSeq;
	m_mapMakeFun["www-authenticate"] = MakeAuth;
	m_mapMakeFun["authorization"] = MakeAuth;
	m_mapMakeFun["subject"] = MakeSubject;
}

/*******************************************************************************
* �������ƣ�	GetMsgLen
* ����������	��ȡ����ĳ���
* ���������	strMsg			-- �յ��ķ�����Ϣ��
* �� �� ֵ��	�����,ֻ����������ʱ���÷���ֵ��Ч��
* ����˵����	���������������ȷ���0
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2012-04-17	�ű�	      	����
*******************************************************************************/
int CMsg::GetMsgLen(const std::string &strMsg) const
{
	std::string::size_type nStart = strMsg.find_first_not_of(" \t\r\n");
	nStart = strMsg.find("\r\n\r\n", nStart);
	if(std::string::npos == nStart)
	{
		return 0;
	}
	nStart += 4;
	std::string strLine = GetContent(strMsg, "Content-Length", ":", "\r\n");
	int nContentLength = atoi(strLine.c_str());
	int nLen = (int)nStart +nContentLength;
	if ((int)strMsg.size() < nLen)
	{
		return 0;
	}
	return nLen;
}

//�������к͸�ͷ��
void CMsg::Parse2Line(const std::string &strMsg)
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
			//ͷ�������
			break;
		}
		if (bFirst)
		{
			m_strFirstLine = strLine;
			bFirst = false;
			//Log("���У�<%s>", m_strFirstLine.c_str());
			continue;
		}
		std::string::size_type nColon = strLine.find(':');
		if (std::string::npos == nColon)
		{
			Log(Tool::Error, "�����Ҳ���ͷ������<%s>", strLine.c_str());
			continue;
		}
		std::string strHead = strLine.substr(0, nColon);
		std::string strContent = GetContent(strLine, strHead, ":", "\r\n");
		//Log("<%s> - <%s>", strHead.c_str(), strContent.c_str());
		m_vecHead.push_back(std::make_pair(strHead, strContent));
	}
}


//��װͷ������
void CMsg::MakeHeadText(std::string strHead, std::string &strText)
{
	std::transform(strHead.begin(), strHead.end(), strHead.begin(), tolower);
	std::unordered_map<std::string, CT_Make>::iterator it = m_mapMakeFun.find(strHead);
	if (m_mapMakeFun.end() != it)
	{
		it->second(this, strHead, strText);
	}
}

//��ȡͷ����������ַ���
std::string CMsg::GetContent( const std::string &strMsg, std::string strName, const std::string &strPrefix, 
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


//�����������ַ���ת��Ϊö��ֵ
int CMsg::Str2Type(bool bReply, const std::string &strCmd)
{
	std::unordered_map<std::string, int> mapType;
	if (!bReply)
	{
		mapType["REGISTER"] = eCmdRegister;
		mapType["MESSAGE"] = eCmdMessage;
		mapType["INVITE"] = eCmdInvite;
		mapType["BYE"] = eCmdBye;
		mapType["ACK"] = eCmdAck;
		mapType["INFO"] = eCmdInfo;
	}
	else
	{
		mapType["REGISTER"] = eCmdRegisterR;
		mapType["MESSAGE"] = eCmdMessageR;
		mapType["INVITE"] = eCmdInviteR;
		mapType["BYE"] = eCmdByeR;
		//mapType["INFO"] = eCmdInfoR;
	}
	std::unordered_map<std::string, int>::iterator it = mapType.find(strCmd);
	if (mapType.end() != it)
	{
		return it->second;
	}
	return eCmdNone;
}

//����Via
void CMsg::ParseVia(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ��SIP/2.0/UDP Դ������IP:�˿�;rport=;branch=...
	//���磺 SIP/2.0/UDP 192.168.4.41:5060;rport=5060;branch=z9hG4bK1821791783
	ViaNode node;
	node.strAddr = GetContent(strText, " ", "", ":", ";");
	node.nPort = atoi(GetContent(strText, node.strAddr, ":", ";").c_str());
	node.nPortR = atoi(GetContent(strText, "rport", "=", ";").c_str());
	node.strBranch = GetContent(strText, "branch", "=", ";");
	pThis->m_vecVia.push_back(node);
}

//����From��To
void CMsg::ParseFromTo(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ�����磺<sip:34010000491122209012@3401000049>;tag=703701342
	FromToNode &node = (0 == strHead.compare("to") ? pThis->m_To :pThis-> m_From);
	node.strID = GetContent(strText, "<sip", ":", "@");
	node.strDomain = GetContent(strText, node.strID, "@", ">", ";");
	node.strTag = GetContent(strText, "tag", "=", ";");
}

//����CSeq
void CMsg::ParseCSeq(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ�����磺CSeq: 1 REGISTER
	CSeqNode &node = pThis->m_CSeq;
	node.nNo = atoi(GetContent(strText, "", "", " ").c_str());
	node.strCmd = GetContent(strText, " ", "", ";");
}

//����Contact
void CMsg::ParseContact(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ�����磺 <sip:34010000491122209012@192.168.4.41:5060>
	ContactNode &node = pThis->m_Contact;
	node.strID = GetContent(strText, "<sip", ":", "@");
	node.strIP = GetContent(strText, node.strID, "@", ":");
	node.nPort = atoi(GetContent(strText, node.strIP, ":", ";", ">").c_str());
}

//����WWW-Authenticatie����Authorization
void CMsg::ParseAuth(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ�����磺WWW-Authenticate: Digest realm="64010000",nonce="6fe9ba44a76be22a"
	//���磺Authorization: Digest username="64010000002020000001", realm="64010000",
	//nonce="6fe9ba44a76be22a", uri="sip:64010000002000000001@172.18.16.5:5060",
	//	response="9625d92d1bddea7a911926e0db054968", algorithm=MD5
	AuthNode &node = pThis->m_Auth;
	node.strAlgorithm = GetContent(strText, "", "", " ");
	node.strNonce = GetContent(strText, "nonce", "=\"", "\"");
	node.strRealm = GetContent(strText, "realm", "=\"", "\"");
	node.strUri = GetContent(strText, "uri", "=\"", "\"");
	node.strUsername = GetContent(strText, "username", "=\"", "\"");
	node.strResponse = GetContent(strText, "response", "=\"", "\"");
	node.strOpaque = GetContent(strText, "opaque", "=\"", "\"");
	node.strEncrypt = GetContent(strText, "algorithm", "=", "," );
}

//����Subject
void CMsg::ParseSubject(CMsg *pThis, const std::string &strHead, const std::string &strText)
{
	//Text��ʽ��ý����������ID:���Ͷ�ý�������,ý�������ID:���ն�ý�������
	SubjectNode &node = pThis->m_Subject;
	node.strSenderID = GetContent(strText, "", ":", ",");
	node.strSenderNo = GetContent(strText, node.strSenderID, ":", ",");
	node.strReceiverID = GetContent(strText, node.strSenderNo, ",", ":", ",");
	node.strReceiverNo = GetContent(strText, node.strReceiverID, ":", ",");
}


//��װVia
void CMsg::MakeVia(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	for (size_t i = 0; i < pThis->m_vecVia.size(); ++i)
	{
		ViaNode &node = pThis->m_vecVia[i];
		std::ostringstream oss;
		oss  << "SIP/2.0/UDP " << node.strAddr;
		if (0 != node.nPort)
		{
			oss << ":" << node.nPort;
		}
		oss << ";" << "rport";
		if (0 != node.nPortR)
		{
			oss << "=" << node.nPortR;
		}
		oss << ";";
		if (node.strBranch.empty())
		{
			node.strBranch = GetBranch();
		}		
		oss << "branch=" << node.strBranch;// << ";";
		strText.append(oss.str());
	}

}

//��װFrom\To
void CMsg::MakeFromTo(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	FromToNode &node = "to" == strHead ? pThis->m_To : pThis->m_From;
	std::ostringstream oss;
	oss << "<sip:" << node.strID << "@" << node.strDomain << ">";
	if (!node.strTag.empty())
	{
		oss << ";tag=" << node.strTag;
	}
	strText = oss.str();
}

//��װCSeq
void CMsg::MakeCSeq(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	CSeqNode &node = pThis->m_CSeq;
	std::ostringstream oss;
	oss << node.nNo << " " << node.strCmd;
	strText = oss.str();
}

//��װContact
void CMsg::MakeContact(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	ContactNode &node = pThis->m_Contact;
	std::ostringstream oss;
	oss << "<sip:" << node.strID << "@" << node.strIP << ":" << node.nPort << ">";
	strText = oss.str();

}

//��װWWW-Authenticatie����Authorization
void CMsg::MakeAuth(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	AuthNode &node = pThis->m_Auth;
	std::ostringstream oss;
	oss << node.strAlgorithm << " ";
	if (!node.strUsername.empty())
	{
		oss << "username=\"" << node.strUsername << "\",";
	}
	if (!node.strRealm.empty())
	{
		oss << "realm=\"" << node.strRealm << "\",";
	}
	if (!node.strNonce.empty())
	{
		oss << "nonce=\"" << node.strNonce << "\",";
	}
	if (!node.strUri.empty())
	{
		oss << "uri=\"" << node.strUri << "\",";
	}
	if (!node.strOpaque.empty())
	{
		oss << "opaque=\"" << node.strOpaque << "\",";
	}
	if (!node.strResponse.empty())
	{
		oss << "response=\"" << node.strResponse << "\",";
	}
	if (!node.strEncrypt.empty())
	{
		oss << "algorithm=" << node.strEncrypt;
	}
	strText = oss.str();
}

//��װSubject
void CMsg::MakeSubject(CMsg *pThis, const std::string &strHead, std::string &strText)
{
	SubjectNode &node = pThis->m_Subject;
	std::ostringstream oss;
	oss << node.strSenderID << ":" << node.strSenderNo << "," ;
	oss << node.strReceiverID << ":" << node.strReceiverNo;
	strText = oss.str();
}

//����һ��GUID�ַ���
std::string CMsg::GUIDStr()
{
#ifdef _WIN32
	GUID guid;    
	if   (SUCCEEDED(::CoCreateGuid(&guid)))   
	{      
		std::ostringstream oss;
		oss << std::hex << guid.Data1 << "-" << std::hex << guid.Data2 << "-"
			<< std::hex << guid.Data3 << "-" << std::hex << (short)guid.Data4[0]
			<< std::hex << (short)guid.Data4[1] << std::hex << (short)guid.Data4[2]
			<< std::hex << (short)guid.Data4[3] << std::hex << (short)guid.Data4[4]
			<< std::hex << (short)guid.Data4[5] << std::hex <<	(short)guid.Data4[6]
			<< std::hex << (short)guid.Data4[7];
		return oss.str();
	}	
#else
	uuid_t uuid;
	char str[36];

	uuid_generate(uuid);
	uuid_unparse(uuid, str);

	std::string strUuid = str;
	return strUuid;
#endif
	Log("����GUIDʧ�ܣ�");	
	return std::string();
}

//����һ��Sip2.0�й涨��ʽ��Branch
std::string CMsg::GetBranch()
{
	std::ostringstream oss;
	oss << "z9hG4bK" << GUIDStr();
	return oss.str();
}


/*******************************************************************************
* �������ƣ�	
* ����������	��ȡ��Ȩ��Ϣ�ַ���
* ���������	strAlgorithm	-- ��Ȩ�㷨
*				strMethod		-- ����
*				strUri			-- URI
*				strUser			-- �û���
*				strPassword		-- ����
*				strReaml		-- ��Ȩ��
*				strNonce		-- ��Ȩ��ʱ����
* ���������	
* �� �� ֵ��	��Ȩ�ַ�����
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2013-07-19	�ű�	      	����
*******************************************************************************/
std::string CMsg::Authorization(std::string strAlgorithm, const std::string &strMethod,
							   const std::string &strUri, const std::string &strUser, 
							   const std::string &strPassword, const std::string &strRealm,
							   const std::string &strNonce )
{ 
	if (strRealm.empty() || strUser.empty() || strPassword.empty())
	{
		return "";
	}

	//for test 
	//std::string strNonce1 = "1536753022:b2952e403ec98ecd4ea0b97a364146eb";
	//test end

	Log(Tool::Debug, "strAlgorithm=[%s], strMethod[%s],strUri[%s],\nstrUser[%s],strPassword[%s],strRealm[%s],strNonce[%s]\n",
		strAlgorithm.c_str(), strMethod.c_str(), strUri.c_str(), strUser.c_str(), strPassword.c_str(), strRealm.c_str(), strNonce.c_str());
	Rtsp::CEncrypt encrypt;
	std::string str;
	std::transform(strAlgorithm.begin(), strAlgorithm.end(), strAlgorithm.begin(), tolower);
	if (0 == strAlgorithm.compare("digest") || 0 == strAlgorithm.compare("Digest"))
	{
		//���ּ���ģʽ
		//responseΪmd5(md5(username:realm:password):nonce:md5(cmd:url))

		//md5(username:realm:password)
		char buff1[33] = {0};
		std::ostringstream oss;
		oss << strUser << ":" << strRealm << ":" << strPassword;
		str = oss.str();
		encrypt.MD5Data((const byte *)str.c_str(), str.size(), buff1);

		//Log("str[%s], buf[%s]", str.c_str(), buff1);

		//md5(cmd:url)
		char buff2[33] = {0};
		oss.clear();
		oss.str("");
		oss << strMethod << ":" << strUri;
		str = oss.str();
		encrypt.MD5Data((const byte *)str.c_str(), str.size(), buff2);

		//Log("str[%s], buf[%s]", str.c_str(), buff2);

		//md5(buff1:nonce:buff2)
		char buff3[33] = {0};
		oss.clear();
		oss.str("");
		oss << buff1 << ":" << strNonce << ":" << buff2;
		str = oss.str();
		encrypt.MD5Data((const byte *)str.c_str(), str.size(), buff3);
		//Log("str[%s], buf[%s]", str.c_str(), buff3);

		//��������Ϣ
		str = buff3;
	}
	else {

	}

	Log("reponese[%s]\n", str.c_str());
	return str;
}

//
//��������
//

CMsgBase::CMsgBase(void)
{
}

CMsgBase::~CMsgBase(void)
{
}

//��������
void CMsgBase::ParseFirstLine(const std::string &strLine)
{
	std::string::size_type nStart = strLine.find_first_not_of(" \t\r\n");
	if (std::string::npos == nStart)
	{
		m_FirstLine.strCmd = eCmdNone;
		return;
	}
	int nEnd = strLine.find(" ", nStart);
	if (std::string::npos == nEnd)
	{
		m_FirstLine.strCmd = eCmdNone;
		return;
	}
	 m_FirstLine.strCmd = strLine.substr(nStart, nEnd-nStart);
	 nStart = nEnd+1;
	 nEnd = strLine.find(" ", nStart);
	 if (std::string::npos == nEnd)
	 {
		 nEnd = strLine.size();
	 }
	 std::string strUri = strLine.substr(nStart, nEnd-nStart);
	 m_FirstLine.strID = GetContent(strUri, "sip", ":", "@");
	 m_FirstLine.strAddr = GetContent(strUri, m_FirstLine.strID, "@", ":");
	 m_FirstLine.nPort = atoi(GetContent(strUri, m_FirstLine.strAddr, ":", " ").c_str());
}

//��ȡ��Ϣ����
int CMsgBase::GetCmdType()
{
	return Str2Type(false, m_FirstLine.strCmd);
}

//���������ַ���
void CMsgBase::MakeFirstLine()
{
	std::ostringstream oss;
	oss << m_FirstLine.strCmd << " " << "sip:" << m_FirstLine.strID << "@" << m_FirstLine.strAddr;
	if (0 != m_FirstLine.nPort)
	{
		oss << ":" << m_FirstLine.nPort;
	}
	oss << " SIP/2.0";
	m_strFirstLine = oss.str();
}

//
//Ӧ������
//

CMsgRBase::CMsgRBase(void)
{
	m_mapDesc[eRTrying] = "Trying";
	m_mapDesc[eROK] = "OK";
	m_mapDesc[eRBadRequest] = "Bad Request";
	m_mapDesc[eRUnAuthorize] = "Unauthorized";
	m_mapDesc[eRForbidden] = "Forbidden";
	m_mapDesc[eRNotFound] = "Not Found";
	m_mapDesc[eRNotExist] = "Call/Transaction Does Not Exist";
	m_mapDesc[eRNotAcceptable] = "Not Acceptable Here";
	m_mapDesc[eRServerError] = "Server Internal Err";
}

CMsgRBase::~CMsgRBase(void)
{

}

//��������
void CMsgRBase::ParseFirstLine(const std::string &strLine)
{
	std::string::size_type nStart = strLine.find_first_not_of(" \t\r\n");
	if (std::string::npos == nStart)
	{
		return;
	}
	nStart = strLine.find(" ", nStart);
	if (std::string::npos == nStart)
	{
		return;
	}
	nStart += 1;
	std::string::size_type nEnd = strLine.find(" ", nStart);
	if (std::string::npos == nStart)
	{
		return;
	}
	m_FirstLine.nReply = atoi(strLine.substr(nStart, nEnd-nStart).c_str());
	nStart = nEnd+1;
	m_FirstLine.strReply = strLine.substr(nStart, strLine.size()-nStart);
}

//��ȡ��Ϣ����
int CMsgRBase::GetCmdType()
{
	return Str2Type(true, m_CSeq.strCmd);
}

//���������ַ���
void CMsgRBase::MakeFirstLine()
{
	std::ostringstream oss;
	oss << "SIP/2.0 " << m_FirstLine.nReply << " " << m_FirstLine.strReply;
	m_strFirstLine = oss.str();
}

//������������Ӧ����Ϣ
void CMsgRBase::Reply(int nReply, CMsgBase *pMsgOrg, const std::string &strContent /* = "" */)
{
	m_nType = Str2Type(true, pMsgOrg->m_CSeq.strCmd);
	m_FirstLine.nReply = nReply;
	m_FirstLine.strReply = m_mapDesc[nReply];
	m_vecVia = pMsgOrg->m_vecVia;
	if (m_vecVia.size() > 0)
	{
		m_vecVia[0].nPortR = pMsgOrg->m_nSrcPort;
	}
	m_From = pMsgOrg->m_From;
	m_To = pMsgOrg->m_To;
	m_CSeq = pMsgOrg->m_CSeq;
	m_Auth = pMsgOrg->m_Auth;
	m_Contact = pMsgOrg->m_Contact;
	m_strContent = strContent;
	m_vecHead.push_back(std::make_pair(STR_HEAD_VIA, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_FROM, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_TO, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CALLID, pMsgOrg->GetHead(STR_HEAD_CALLID)));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CSEQ, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_USER_AGENET, "Syz"));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_LENGTH, ""));
}


//
//ע����Ϣ
//

CMsgReg::CMsgReg()
{
	m_nType = eCmdRegister;
	m_FirstLine.strCmd = "REGISTER";
	m_CSeq.strCmd = m_FirstLine.strCmd;	
};

//���ò���
void CMsgReg::SetParma( std::string strServerID, std::string strLocalID, std::string strLocalIP, 
					   int nLocalPort, int nCSeq, const AuthNode &auth, const std::string &strPassword,
					   int nExpires /* = 3600 */, std::string strFromTag /* = "" */, std::string strCallID /* = "" */)
{
	ViaNode node;
	node.strAddr = strLocalIP;
	node.nPort = nLocalPort;
	node.strBranch = GetBranch();
	m_vecVia.push_back(node);
	m_FirstLine.strID = strServerID;
	m_FirstLine.strAddr = strServerID.substr(0, DOMAIN_COUNT);	
	m_From.strID = strLocalID;
	m_From.strDomain = strLocalID.substr(0, DOMAIN_COUNT);
	m_To = m_From;
	m_From.strTag = strFromTag.empty() ? GUIDStr() : strFromTag;
	m_CSeq.nNo = nCSeq;
	m_CSeq.strCmd = m_FirstLine.strCmd;
	m_Contact.strID = strLocalID;
	m_Contact.strIP = strLocalIP;
	m_Contact.nPort = nLocalPort;
	m_Auth = auth;
	if (!m_Auth.strRealm.empty() && !m_Auth.strNonce.empty())
	{
		if (m_Auth.strUsername.empty())
		{
			m_Auth.strUsername = strLocalID;
		}
		if (m_Auth.strEncrypt.empty())
		{
			m_Auth.strEncrypt = "MD5";
		}
		if (m_Auth.strAlgorithm.empty())
		{
			m_Auth.strAlgorithm = "Digest";
		}
		if (m_Auth.strUri.empty())
		{
			std::ostringstream oss;
			oss << "sip:" << strServerID << "@" << strServerID.substr(0, DOMAIN_COUNT);
			m_Auth.strUri = oss.str();
		}
		if (m_Auth.strResponse.empty())
		{
			m_Auth.strResponse = Authorization(
				m_Auth.strAlgorithm,
				m_FirstLine.strCmd, 
				m_Auth.strUri,
				m_Auth.strUsername, 
				strPassword,
				m_Auth.strRealm,
				m_Auth.strNonce
				);			
		}
	}
	m_vecHead.clear();
	m_vecHead.push_back(std::make_pair(STR_HEAD_VIA, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_FROM, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_TO, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CALLID, strCallID.empty() ? GUIDStr() : strCallID));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CSEQ, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTACT, ""));
	if (!m_Auth.strResponse.empty())
	{
		m_vecHead.push_back(std::make_pair(STR_HEAD_AUTHORIZATION,""));
	}
	m_vecHead.push_back(std::make_pair(STR_HEAD_MAX_FORWARD,"70"));
	std::ostringstream oss;
	oss << nExpires;
	m_vecHead.push_back(std::make_pair(STR_HEAD_EXPIRES,oss.str()));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_LENGTH,""));
}

CMsgMsg::CMsgMsg()
{
	m_nType = eCmdMessage;
	m_FirstLine.strCmd = "MESSAGE";
	m_CSeq.strCmd = m_FirstLine.strCmd;
}

//���ò���
void CMsgMsg::SetParam( const std::string &strToID, const std::string &strLocalID, const std::string &strLocalIP,
					   int nLocalPort, int nCSeq, const std::string &strContent, 
					   const std::string &strCallID /* = "" */, const std::string &strFromTag /* = "" */ )
{
	m_FirstLine.strID = strToID;
	m_FirstLine.strAddr = strToID.substr(0, DOMAIN_COUNT);
	ViaNode node;
	node.strAddr = strLocalIP;
	node.nPort = nLocalPort;
	m_vecVia.push_back(node);
	m_To.strID = strToID;
	m_To.strDomain = strToID.substr(0, DOMAIN_COUNT);
	m_From.strID = strLocalID;
	m_From.strDomain = strLocalID.substr(0, DOMAIN_COUNT);
	m_From.strTag = strFromTag.empty() ? GUIDStr() : strFromTag;
	m_CSeq.nNo = nCSeq;
	m_strContent = strContent;
	m_vecHead.clear();
	m_vecHead.push_back(std::make_pair(STR_HEAD_VIA, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_TO, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_FROM, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CSEQ, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CALLID, strCallID.empty() ? GUIDStr() : strCallID));
	m_vecHead.push_back(std::make_pair(STR_HEAD_MAX_FORWARD, "70"));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_LENGTH, ""));
	if (!m_strContent.empty())
	{
		m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_TYPE, "Application/MANSCDP+xml"));
	}
}

//
//BYE��Ϣ
//

CMsgBye::CMsgBye()
{
	m_nType = eCmdBye;
	m_FirstLine.strCmd = "BYE";
	m_CSeq.strCmd = m_FirstLine.strCmd;
}

void CMsgBye::SetParam( const std::string &strToID, const std::string &strToTag, const std::string &strLocalID,
					   const std::string &strLocalIP, int nLocalPort, const std::string &strFromTag,
					   int nCSeq, const std::string &strCallID )
{
	m_FirstLine.strID = strToID;
	m_FirstLine.strAddr = strToID.substr(0, DOMAIN_COUNT);
	ViaNode node;
	node.strAddr = strLocalIP;
	node.nPort = nLocalPort;
	m_vecVia.push_back(node);
	m_To.strID = strToID;
	m_To.strID = strToID.substr(0, DOMAIN_COUNT);
	m_To.strTag = strToTag;
	m_From.strID = strLocalID;
	m_From.strDomain = strLocalID.substr(0, DOMAIN_COUNT);
	m_From.strTag = strFromTag;
	m_CSeq.nNo = nCSeq;
	m_Contact.strID = strLocalID;
	m_Contact.strIP = strLocalIP;
	m_Contact.nPort = nLocalPort;
	m_vecHead.clear();
	m_vecHead.push_back(std::make_pair(STR_HEAD_VIA, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_TO, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_FROM, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CSEQ, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CALLID, strCallID.empty() ? GUIDStr() : strCallID));
	m_vecHead.push_back(std::make_pair(STR_HEAD_MAX_FORWARD, "70"));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_LENGTH, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTACT, ""));
}

//������������Ӧ����Ϣ
void CMsgInvitR::Reply( int nReply, CMsgBase *pMsgOrg, const std::string &strLocalIP, 
					   int nLocalPort, const std::string &strContent /* = "" */ )
{
	CMsgRBase::Reply(nReply, pMsgOrg, strContent);
	m_Contact.strID = pMsgOrg->m_To.strID;
	m_Contact.strIP = strLocalIP;
	m_Contact.nPort = nLocalPort;
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTACT, ""));
	m_vecHead.push_back(std::make_pair(STR_HEAD_CONTENT_TYPE, "application/sdp"));
}

}

