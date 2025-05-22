#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace Sip
{

	
//sip命令类型(应答类型必须 = 类型+1)
enum
{
	eCmdNone = 0,		//无意义的初始值
	eCmdRegister,		//注册
	eCmdRegisterR,		//注册应答
	eCmdMessage,		//消息
	eCmdMessageR,		//消息应答
	eCmdInvite,			//发起会话
	eCmdInviteR,		//发起会话应答
	eCmdBye,			//结束
	eCmdByeR,			//结束应答
	eCmdAck,			//确认
	eCmdInfo,			//请求历史时使用, 
	//eCmdInfoR,			//请求历史时使用
};

//应答值
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


//Sip消息基类
class CMsg
{
public:

	CMsg();

	virtual ~CMsg();

	//设置消息的源端口
	void SetSrcPort(int nPort){m_nSrcPort = nPort;}

	//测试一个字符串的消息类型
	static int CheckType(const std::string &strMsg);

	//解析命令, 返回命令的长度
	int Parse(const std::string &strMsg);

	//获取指定头域的值
	std::string GetHead(std::string strHead);

	//获取命令字符串
	const std::string & Str(){ return m_strMsg; }

	//获取负载字符串
	const std::string & ContentStr(){return m_strContent;}

	//组装命令
	void Make();

protected:

	//<头域名，值>
	typedef std::pair<std::string, std::string> HeadNode;
	typedef std::vector<HeadNode> HeadVec;

	//解析函数定义
	typedef void (*CT_Parse)(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//组装函数定义
	typedef void (*CT_Make)(CMsg *pThis, const std::string &strHead, std::string &strText);

	//
	//虚函数
	//

	//添加解析函数
	virtual void AddParseFun();

	//添加组装函数
	virtual void AddMakeFun();

	//解析首行
	virtual void ParseFirstLine(const std::string &strLine) = 0;

	//生成首行
	virtual void MakeFirstLine() = 0;

	//获取消息类型
	virtual int GetCmdType() = 0;

	//
	//非虚成员函数
	//

	//获取消息的长度
	int GetMsgLen(const std::string &strMsg) const;

	//拆解出首行和各头域
	void Parse2Line(const std::string &strMsg);

	//组装头域内容
	void MakeHeadText(std::string strHead, std::string &strText);


	//
	//静态成员函数
	//

	//按指定的规则获取值
	static std::string GetContent(
		const std::string &strMsg,				//原始字符串
		std::string strName,					//要查找的前导名字
		const std::string &strPrefix,			//前导名和值之间的字符
		const std::string &strSuffix,			//结束字符
		const std::string &strSuffixSubby = ""	//次优先级结束字符
		);

	//将命令类型字符串转换为枚举值
	static int Str2Type(bool bReply, const std::string &strCmd);

	//解析Via
	static void ParseVia(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//解析From、To
	static void ParseFromTo(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//解析CSeq
	static void ParseCSeq(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//解析Contact
	static void ParseContact(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//解析WWW-Authenticatie或者Authorization
	static void ParseAuth(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//解析Subject
	static void ParseSubject(CMsg *pThis, const std::string &strHead, const std::string &strText);

	//组装Via
	static void MakeVia(CMsg *pThis, const std::string &strHead, std::string &strText);

	//组装From\To
	static void MakeFromTo(CMsg *pThis, const std::string &strHead, std::string &strText);

	//组装CSeq
	static void MakeCSeq(CMsg *pThis, const std::string &strHead, std::string &strText);

	//组装Contact
	static void MakeContact(CMsg *pThis, const std::string &strHead, std::string &strText);

	//组装WWW-Authenticatie或者Authorization
	static void MakeAuth(CMsg *pThis, const std::string &strHead, std::string &strText);

	//组装Subject
	static void MakeSubject(CMsg *pThis, const std::string &strHead, std::string &strText);

	//生成一个gudi
	static std::string GUIDStr();

	//生成branch
	static std::string GetBranch();

	static std::string Authorization(std::string strAlgorithm, const std::string &strMethod,
		const std::string &strUri, const std::string &strUser, 
		const std::string &strPassword, const std::string &strRealm,
		const std::string &strNonce );

public:

	//消息类型
	int m_nType;

	//消息的源端口
	int m_nSrcPort;

	//via头域信息
	struct ViaNode
	{
		std::string strAddr;	//IP或者域
		int nPort;				//端口
		int nPortR;				//rport的值
		std::string strBranch;	//branch
		ViaNode(): nPort(0), nPortR(0){}
	};
	typedef std::vector<ViaNode > ViaVec;		//Via头域列表定义
	ViaVec m_vecVia;

	//From和To头域
	struct FromToNode
	{
		std::string strID;				//ID
		std::string strDomain;			//域
		std::string strTag;				//tag
	};
	FromToNode m_From;
	FromToNode m_To;

	//CSeq头域信息：命令计数
	struct CSeqNode
	{
		int nNo;						//命令计数
		std::string strCmd;				//命令类型
		CSeqNode(): nNo(1){}
	}m_CSeq;

	//Contact头域信息
	struct ContactNode
	{
		std::string strID;
		std::string strIP;
		int nPort;
		ContactNode(): nPort(0){}
	} m_Contact;

	//WWW-authenticate或Authorization头域信息
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
		std::string strSenderID;	//流发送者ID
		std::string strSenderNo;	//流发送者序号
		std::string strReceiverID;	//流接收者ID
		std::string strReceiverNo;	//流接收者序号
	}m_Subject;
protected:

	//消息体字符串
	std::string m_strMsg;

	//首行字符串
	std::string m_strFirstLine;

	//各个头域列表<头域名，值>
	HeadVec m_vecHead;

	//解析函数列表
	std::unordered_map<std::string, CT_Parse> m_mapParseFun;

	//组装函数列表
	std::unordered_map<std::string, CT_Make> m_mapMakeFun;

	//负载字符串
	std::string m_strContent;

};

//Sip主动消息基类
class CMsgBase : public CMsg
{
public:

	CMsgBase(void);

	virtual ~CMsgBase(void);

public:

	//首行信息
	struct FirstLineNode
	{
		std::string strCmd;			//首行中的命令类型
		std::string strID;			//首行uri中的ID
		std::string strAddr;		//首行uri中的IP或者域
		int nPort;					//首行uri中的端口
		FirstLineNode(): nPort(0){}
	}m_FirstLine;

protected:


	//解析首行
	virtual void ParseFirstLine(const std::string &strLine);

	//获取消息类型
	virtual int GetCmdType();

	//生成首行
	virtual void MakeFirstLine();
};

//Sip应答消息基类
class CMsgRBase : public CMsg
{
public:
	CMsgRBase(void);

	virtual ~CMsgRBase(void);

	//根据命令设置应答信息
	void Reply(int nReply, CMsgBase *pMsgOrg, const std::string &strContent = "");

public:

	//首行信息
	struct FirstLineNode
	{
		int nReply;					//应答号
		std::string strReply;		//应答描述
		FirstLineNode(): nReply(eRNone){}
	}m_FirstLine;

protected:

	//解析首行
	virtual void ParseFirstLine(const std::string &strLine);

	//获取消息类型
	virtual int GetCmdType();

	//生成首行
	virtual void MakeFirstLine();

protected:

	//<应答号、应道描述>列表
	std::unordered_map<int, std::string> m_mapDesc;
};

//注册消息
class CMsgReg : public CMsgBase
{
public:

	CMsgReg();

	~CMsgReg(){}

	//设置参数
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

//Message消息
class CMsgMsg : public CMsgBase
{
public:

	CMsgMsg();

	~CMsgMsg(){}

	//设置参数
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

//BYE消息
class CMsgBye : public CMsgBase
{
public:

	CMsgBye();

	~CMsgBye(){}

	//通过一个命令设置BYE的参数
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

//INVITE应答消息
class CMsgInvitR : public CMsgRBase
{
public:

	CMsgInvitR(){}

	~CMsgInvitR(){}

	//根据命令设置应答信息
	void Reply(
		int nReply, 
		CMsgBase *pMsgOrg,
		const std::string &strLocalIP,
		int nLocalPort,
		const std::string &strContent = ""
		);	
};

//录像文件上传消息
class CMsgRecordInfo : public CMsgBase
{
	CMsgRecordInfo() {};
	~CMsgRecordInfo() {};
};

}

