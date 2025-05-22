#pragma once
#include <string>
#include <vector>
#ifdef WIN32
#include "windows/Monitor.h"
#include "windows/TcpClient.h"
#else
#include "linux/Monitor.h"
#include "linux/TcpClient.h"
#endif
#include "Tool/TBuff.h"
#include "Tool/XEvent.h"

//应答值
enum class HttpResponse
{
	eRNone = 0,
	eRSwitchProtocols = 101,
	eROK = 200,
	eRBadRequest = 400,
	eRUnAuthorize = 401,
	eRForbidden = 403,
	eRNotFound = 404,
	eRNotAcceptable = 488,
	eRServerError = 500,
};

class CHttpRespone {
public:
	CHttpRespone() { m_nContentLength = 0; };
	~CHttpRespone() {};

	/*解析msg数据头
	返回值：返回msg头数据长度*/
	int PaserHttpHead(char *buf);

	//拆解出首行和各头域
	bool Parse2Line(const std::string &strMsg);
	////解析首行
	////拆解出首行和各头域
	bool ParseFirstLine(const std::string &strMsg);

	//按指定的规则获取值
	std::string GetContent(
		const std::string &strMsg,				//原始字符串
		std::string strName,					//要查找的前导名字
		const std::string &strPrefix,			//前导名和值之间的字符
		const std::string &strSuffix,			//结束字符
		const std::string &strSuffixSubby = ""	//次优先级结束字符
	);

	//获取指定头域的值
	std::string GetHead(std::string strHead);

	//获取指定头域的值
	void GetHead(std::string strHead, std::vector<std::string> &vecValue);

public:
	//<头域名，值>
	typedef std::pair<std::string, std::string> HeadNode;
	typedef std::vector<HeadNode> HeadVec;

	//各个头域列表<头域名，值>
	HeadVec m_vecHead;
	int		m_nContentLength;

	std::string m_strFirstLine;
	//首行信息
	struct FirstLineNode
	{
		HttpResponse Reply;			//应答号
		std::string strReply;		//应答描述
		FirstLineNode() : Reply(HttpResponse::eRNone) {}
	}m_FirstLine;

	Tool::TBuff<char> m_bufRecvData;
	std::string m_strContent;
};

class CHttp : public CContextBase
{
public:
	CHttp();
	~CHttp();
	int Connect(std::shared_ptr<CMonitor> p,std::string strIp, unsigned short nPort, const char *url, int nMilliseconds = 10000);
	int DisConnect();
	int Post(const char *SendData, int nDataLen, const char* cDataType,int nMilliseconds = 10*1000, const char *Authorization = nullptr);
	int Post(std::vector<std::pair<std::string, std::string>> head,  const char* SendData, int nDataLen, int nMilliseconds = 10 * 1000);
	int Put(const char *SendData, int nDataLen, const char* cDataType, int nMilliseconds = 10 * 1000, const char *Authorization = nullptr);

	int Get(int nMilliseconds = 10 * 1000, const char *Authorization = nullptr);
	int Get(std::vector<std::pair<std::string, std::string>> params, int nMilliseconds = 10 * 1000, const char *Authorization = nullptr);
	int Get(std::vector<std::pair<std::string, std::string>> params, std::vector<std::pair<std::string, std::string>> head, int nMilliseconds = 10 * 1000);
	//tcp数据接收回调
	//static std::pair<bool, int> tcpDataCb_s(std::shared_ptr<CContextBase> pContext, Tool::TBuff<char> &Data);
	//std::pair<bool, int> tcpDataCb(Tool::TBuff<char>& Data);
	static int tcpDataCb_s(std::shared_ptr<CContextBase> pContext, Tool::TBuff<char>& Data);
	int tcpDataCb(Tool::TBuff<char> &Data);
	//tcp 连接状态回调
	static void tcpConnectCb(std::shared_ptr<CContextBase> pContext, bool bConnected);

	bool Unzip(const unsigned char *bufGzip, int nLen, std::string &bufOut);
	//处理接收数据
	int ProcessMsg(Tool::TBuff<char> &Data);

	bool Unzip_();
public:
	bool											m_bPaserHead;
	bool											m_bConneted;
	std::string										m_url;
	std::string										m_ip;
	int												m_nPort;

	int												m_nRecvMsgLen;//已经接收的数据长度
	int												m_nContentLen;//消息长度

	CHttpRespone									m_httpRespone;
private:
	std::shared_ptr<CTcpClient>						m_lpTcpClient;
	
	std::shared_ptr<XEvent>							m_hEvent;
	std::shared_ptr<XEvent>							m_hReciveData;

	Tool::TBuff<char>								m_bufReciveHead;
	std::string										m_bufReciveContent;
};

