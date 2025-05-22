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

//Ӧ��ֵ
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

	/*����msg����ͷ
	����ֵ������msgͷ���ݳ���*/
	int PaserHttpHead(char *buf);

	//�������к͸�ͷ��
	bool Parse2Line(const std::string &strMsg);
	////��������
	////�������к͸�ͷ��
	bool ParseFirstLine(const std::string &strMsg);

	//��ָ���Ĺ����ȡֵ
	std::string GetContent(
		const std::string &strMsg,				//ԭʼ�ַ���
		std::string strName,					//Ҫ���ҵ�ǰ������
		const std::string &strPrefix,			//ǰ������ֵ֮����ַ�
		const std::string &strSuffix,			//�����ַ�
		const std::string &strSuffixSubby = ""	//�����ȼ������ַ�
	);

	//��ȡָ��ͷ���ֵ
	std::string GetHead(std::string strHead);

	//��ȡָ��ͷ���ֵ
	void GetHead(std::string strHead, std::vector<std::string> &vecValue);

public:
	//<ͷ������ֵ>
	typedef std::pair<std::string, std::string> HeadNode;
	typedef std::vector<HeadNode> HeadVec;

	//����ͷ���б�<ͷ������ֵ>
	HeadVec m_vecHead;
	int		m_nContentLength;

	std::string m_strFirstLine;
	//������Ϣ
	struct FirstLineNode
	{
		HttpResponse Reply;			//Ӧ���
		std::string strReply;		//Ӧ������
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
	//tcp���ݽ��ջص�
	//static std::pair<bool, int> tcpDataCb_s(std::shared_ptr<CContextBase> pContext, Tool::TBuff<char> &Data);
	//std::pair<bool, int> tcpDataCb(Tool::TBuff<char>& Data);
	static int tcpDataCb_s(std::shared_ptr<CContextBase> pContext, Tool::TBuff<char>& Data);
	int tcpDataCb(Tool::TBuff<char> &Data);
	//tcp ����״̬�ص�
	static void tcpConnectCb(std::shared_ptr<CContextBase> pContext, bool bConnected);

	bool Unzip(const unsigned char *bufGzip, int nLen, std::string &bufOut);
	//�����������
	int ProcessMsg(Tool::TBuff<char> &Data);

	bool Unzip_();
public:
	bool											m_bPaserHead;
	bool											m_bConneted;
	std::string										m_url;
	std::string										m_ip;
	int												m_nPort;

	int												m_nRecvMsgLen;//�Ѿ����յ����ݳ���
	int												m_nContentLen;//��Ϣ����

	CHttpRespone									m_httpRespone;
private:
	std::shared_ptr<CTcpClient>						m_lpTcpClient;
	
	std::shared_ptr<XEvent>							m_hEvent;
	std::shared_ptr<XEvent>							m_hReciveData;

	Tool::TBuff<char>								m_bufReciveHead;
	std::string										m_bufReciveContent;
};

