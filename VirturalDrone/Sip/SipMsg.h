#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace Sip
{

	
//sip��������(Ӧ�����ͱ��� = ����+1)
enum
{
	eCmdNone = 0,		//������ĳ�ʼֵ
	eCmdRegister,		//ע��
	eCmdRegisterR,		//ע��Ӧ��
	eCmdMessage,		//��Ϣ
	eCmdMessageR,		//��ϢӦ��
	eCmdInvite,			//����Ự
	eCmdInviteR,		//����ỰӦ��
	eCmdBye,			//����
	eCmdByeR,			//����Ӧ��
	eCmdAck,			//ȷ��
	eCmdInfo,			//������ʷʱʹ��, 
	//eCmdInfoR,			//������ʷʱʹ��
};

//Ӧ��ֵ
enum 
{
	eRNone = 0,
	eRTrying = 100,
	eROK = 200,
	eRBadRequest = 400,
	eRUnAuthorize = 401,
	eRForbidden = 403,
	eRNotFound = 404,
	eRNotExist = 481,
	eRNotAcceptable = 488,
	eRServerError = 500,
};


#define STR_HEAD_VIA "Via"
#define STR_HEAD_FROM "From"
#define STR_HEAD_TO "To"
#define STR_HEAD_CALLID "Call-ID"
#define STR_HEAD_CSEQ "CSeq"
#define STR_HEAD_CONTACT "Contact"
#define STR_HEAD_WWWAUTHENNTICATE "WWW-Authenticate"
#define STR_HEAD_AUTHORIZATION "Authorization"
#define STR_HEAD_MAX_FORWARD "Max-Forwards"
#define STR_HEAD_EXPIRES "Expires"
#define STR_HEAD_CONTENT_LENGTH "Content-Length"
#define STR_HEAD_USER_AGENET "User-Agent"
#define STR_HEAD_CONTENT_TYPE "Content-Type"


//Sip��Ϣ����
class CMsg
{
public:

	CMsg();

	virtual ~CMsg();

	//������Ϣ��Դ�˿�
	void SetSrcPort(int nPort){m_nSrcPort = nPort;}

	//����һ���ַ�������Ϣ����
	static int CheckType(const std::string &strMsg);

	//��������, ��������ĳ���
	int Parse(const std::string &strMsg);

	//��ȡָ��ͷ���ֵ
	std::string GetHead(std::string strHead);

	//��ȡ�����ַ���
	const std::string & Str(){ return m_strMsg; }

	//��ȡ�����ַ���
	const std::string & ContentStr(){return m_strContent;}

	//��װ����
	void Make();

protected:

	//<ͷ������ֵ>
	typedef std::pair<std::string, std::string> HeadNode;
	typedef std::vector<HeadNode> HeadVec;

	//������������
	typedef void (*CT_Parse)(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//��װ��������
	typedef void (*CT_Make)(CMsg *pThis, const std::string &strHead, std::string &strText);

	//
	//�麯��
	//

	//��ӽ�������
	virtual void AddParseFun();

	//�����װ����
	virtual void AddMakeFun();

	//��������
	virtual void ParseFirstLine(const std::string &strLine) = 0;

	//��������
	virtual void MakeFirstLine() = 0;

	//��ȡ��Ϣ����
	virtual int GetCmdType() = 0;

	//
	//�����Ա����
	//

	//��ȡ��Ϣ�ĳ���
	int GetMsgLen(const std::string &strMsg) const;

	//�������к͸�ͷ��
	void Parse2Line(const std::string &strMsg);

	//��װͷ������
	void MakeHeadText(std::string strHead, std::string &strText);


	//
	//��̬��Ա����
	//

	//��ָ���Ĺ����ȡֵ
	static std::string GetContent(
		const std::string &strMsg,				//ԭʼ�ַ���
		std::string strName,					//Ҫ���ҵ�ǰ������
		const std::string &strPrefix,			//ǰ������ֵ֮����ַ�
		const std::string &strSuffix,			//�����ַ�
		const std::string &strSuffixSubby = ""	//�����ȼ������ַ�
		);

	//�����������ַ���ת��Ϊö��ֵ
	static int Str2Type(bool bReply, const std::string &strCmd);

	//����Via
	static void ParseVia(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//����From��To
	static void ParseFromTo(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//����CSeq
	static void ParseCSeq(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//����Contact
	static void ParseContact(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//����WWW-Authenticatie����Authorization
	static void ParseAuth(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//����Subject
	static void ParseSubject(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//��װVia
	static void MakeVia(CMsg *pThis, const std::string &strHead, std::string &strText);

	//��װFrom\To
	static void MakeFromTo(CMsg *pThis, const std::string &strHead, std::string &strText);

	//��װCSeq
	static void MakeCSeq(CMsg *pThis, const std::string &strHead, std::string &strText);

	//��װContact
	static void MakeContact(CMsg *pThis, const std::string &strHead, std::string &strText);

	//��װWWW-Authenticatie����Authorization
	static void MakeAuth(CMsg *pThis, const std::string &strHead, std::string &strText);

	//��װSubject
	static void MakeSubject(CMsg *pThis, const std::string &strHead, std::string &strText);

	//����һ��gudi
	static std::string GUIDStr();

	//����branch
	static std::string GetBranch();

	static std::string Authorization(std::string strAlgorithm, const std::string &strMethod,
		const std::string &strUri, const std::string &strUser, 
		const std::string &strPassword, const std::string &strRealm,
		const std::string &strNonce );

public:

	//��Ϣ����
	int m_nType;

	//��Ϣ��Դ�˿�
	int m_nSrcPort;

	//viaͷ����Ϣ
	struct ViaNode
	{
		std::string strAddr;	//IP������
		int nPort;				//�˿�
		int nPortR;				//rport��ֵ
		std::string strBranch;	//branch
		ViaNode(): nPort(0), nPortR(0){}
	};
	typedef std::vector<ViaNode > ViaVec;		//Viaͷ���б���
	ViaVec m_vecVia;

	//From��Toͷ��
	struct FromToNode
	{
		std::string strID;				//ID
		std::string strDomain;			//��
		std::string strTag;				//tag
	};
	FromToNode m_From;
	FromToNode m_To;

	//CSeqͷ����Ϣ���������
	struct CSeqNode
	{
		int nNo;						//�������
		std::string strCmd;				//��������
		CSeqNode(): nNo(1){}
	}m_CSeq;

	//Contactͷ����Ϣ
	struct ContactNode
	{
		std::string strID;
		std::string strIP;
		int nPort;
		ContactNode(): nPort(0){}
	} m_Contact;

	//WWW-authenticate��Authorizationͷ����Ϣ
	struct AuthNode
	{
		std::string strAlgorithm;
		std::string strUsername;
		std::string strRealm;
		std::string strNonce;
		std::string strUri;
		std::string strResponse;
		std::string strEncrypt;
		std::string strOpaque;
	}m_Auth;

	struct SubjectNode
	{
		std::string strSenderID;	//��������ID
		std::string strSenderNo;	//�����������
		std::string strReceiverID;	//��������ID
		std::string strReceiverNo;	//�����������
	}m_Subject;
protected:

	//��Ϣ���ַ���
	std::string m_strMsg;

	//�����ַ���
	std::string m_strFirstLine;

	//����ͷ���б�<ͷ������ֵ>
	HeadVec m_vecHead;

	//���������б�
	std::unordered_map<std::string, CT_Parse> m_mapParseFun;

	//��װ�����б�
	std::unordered_map<std::string, CT_Make> m_mapMakeFun;

	//�����ַ���
	std::string m_strContent;

};

//Sip������Ϣ����
class CMsgBase : public CMsg
{
public:

	CMsgBase(void);

	virtual ~CMsgBase(void);

public:

	//������Ϣ
	struct FirstLineNode
	{
		std::string strCmd;			//�����е���������
		std::string strID;			//����uri�е�ID
		std::string strAddr;		//����uri�е�IP������
		int nPort;					//����uri�еĶ˿�
		FirstLineNode(): nPort(0){}
	}m_FirstLine;

protected:


	//��������
	virtual void ParseFirstLine(const std::string &strLine);

	//��ȡ��Ϣ����
	virtual int GetCmdType();

	//��������
	virtual void MakeFirstLine();
};

//SipӦ����Ϣ����
class CMsgRBase : public CMsg
{
public:
	CMsgRBase(void);

	virtual ~CMsgRBase(void);

	//������������Ӧ����Ϣ
	void Reply(int nReply, CMsgBase *pMsgOrg, const std::string &strContent = "");

public:

	//������Ϣ
	struct FirstLineNode
	{
		int nReply;					//Ӧ���
		std::string strReply;		//Ӧ������
		FirstLineNode(): nReply(eRNone){}
	}m_FirstLine;

protected:

	//��������
	virtual void ParseFirstLine(const std::string &strLine);

	//��ȡ��Ϣ����
	virtual int GetCmdType();

	//��������
	virtual void MakeFirstLine();

protected:

	//<Ӧ��š�Ӧ������>�б�
	std::unordered_map<int, std::string> m_mapDesc;
};

//ע����Ϣ
class CMsgReg : public CMsgBase
{
public:

	CMsgReg();

	~CMsgReg(){}

	//���ò���
	void SetParma(
		std::string strServerID,
		std::string strLocalID,
		std::string strLocalIP,
		int nLocalPort,
		int nCSeq,
		const AuthNode &auth,
		const std::string &strPassword,
		int nExpires = 3600,
		std::string strFromTag = "",
		std::string strCallID = ""
		);
};

//Message��Ϣ
class CMsgMsg : public CMsgBase
{
public:

	CMsgMsg();

	~CMsgMsg(){}

	//���ò���
	void SetParam(
		const std::string &strToID,
		const std::string &strLocalID,
		const std::string &strLocalIP,
		int nLocalPort,
		int nCSeq,
		const std::string &strContent,
		const std::string &strCallID = "",
		const std::string &strFromTag = ""
		);
};

//BYE��Ϣ
class CMsgBye : public CMsgBase
{
public:

	CMsgBye();

	~CMsgBye(){}

	//ͨ��һ����������BYE�Ĳ���
	void SetParam(
		const std::string &strToID,
		const std::string &strToTag,
		const std::string &strLocalID,
		const std::string &strLocalIP,
		int nLocalPort,
		const std::string &strFromTag,
		int nCSeq,
		const std::string &strCallID
		);
};

//INVITEӦ����Ϣ
class CMsgInvitR : public CMsgRBase
{
public:

	CMsgInvitR(){}

	~CMsgInvitR(){}

	//������������Ӧ����Ϣ
	void Reply(
		int nReply, 
		CMsgBase *pMsgOrg,
		const std::string &strLocalIP,
		int nLocalPort,
		const std::string &strContent = ""
		);	
};

//¼���ļ��ϴ���Ϣ
class CMsgRecordInfo : public CMsgBase
{
	CMsgRecordInfo() {};
	~CMsgRecordInfo() {};
};

}

