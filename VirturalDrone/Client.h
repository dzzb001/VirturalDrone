#pragma once
#include "Sip/SipMsg.h"
#include "Sip/SubMsg.h"
#include "UdpM_G.h"

#include "Rtcp.h"

#include "TcpClient.h"
#include "TcpServer.h"
//#include "RtspClient.h"
#include "Media/MeadiaPaser.h"

//Rtp发送开始/停止通知回调函数定义
typedef void (*CT_RtpNotify)(LPVOID lpContext,	//环境变量
						 void *pClient,			//发出通知的客户端指针
						 bool bStart			//true：开始发送Rtp，false：停止发送Rtp
						 );

//typedef void(*CT_RtpRecv)(void * lpContext,	//环境变量
//						  unsigned char *pData,						//发出通知的客户端指针
//						  unsigned int nSize
//	);

class CClient : public CContextBase
{
public:

//tcp被动传输时，远端地址输出
typedef void(*CT_Addr)(
	LPVOID lpContext,							//输入，环境变量
	const std::string &strCallID,				//输入，会话ID
	const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
	int nRemotePort								//输入，远端接收Rtp的端口
	);

	CClient(
		const std::string &strDevID,	//所属设备的ID
		const std::string &strPayload	//Rtp的负载数据类型
		);

	CClient::CClient();

	~CClient(void);

	//设置参数
	void SetParam(
		const std::string &strDevID,	//所属设备的ID
		const std::string &strPayload	//Rtp的负载数据类型
	);

	void SetParam(
		const std::string &strDevID,	//所属设备的ID
		const std::string &strPayload,	//Rtp的负载数据类型
		const std::string &strRtspUrl	//前端视频源设备url
	);
	//注册Rtp发送通知回调函数
	void RegCB(CT_RtpNotify pfnRtp, CT_Addr pfnAddr, LPVOID lpContext);

	//命令输入
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//该会话是否已经僵死
	bool Dead();

	//Rtp数据输入
	void RtpIn(char *pBuff, int nLen, int nPayloadLen);

	//主动关闭
	bool Close();

	//处理内部逻辑
	void Process() {};

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient);

	//创建rtp对象并初始化
	bool CreateUdp( int nPort, std::string strLocalIP, std::shared_ptr<CUdpM> &udp, int nType);
	bool DestoryUdp(std::shared_ptr<CUdpM> &udp);

	//rtp 数据回调
	static void RtpUdpDataCB(
		std::shared_ptr<void> pContext,	//环境变量
		const char *pData,							//接收到的数据指针
		int nLen,									//接收到的数据字节数
		const std::string &strFromIP,				//数据源IP
		short nFromPort								//数据源端口
	);

	//rtcp 数据回调
	static void RtcpUdpDataCB(
		std::shared_ptr<void> pContext,	//环境变量
		const char *pData,							//接收到的数据指针
		int nLen,									//接收到的数据字节数
		const std::string &strFromIP,				//数据源IP
		short nFromPort								//数据源端口
	);

	static void RtpDataCB(void* lpContext,	//环境变量
		unsigned char *pData,			    //发出通知的客户端指针
		unsigned int nSize
		);

	//设置rtsp向客户的发送rtp数据
	//void AttchRtsp(std::shared_ptr<CRtspClient> pRtsp);

	static void tcpStartCallback(
		std::shared_ptr<void> pContext,	//环境变量
		int nTimer				//定时器ID
	);
protected:

	//所属设备ID
	std::string m_strDevID;


	//负载类型
	std::string m_strPayload;

	//前端视频源设备url
	std::string m_strRtspUrl;

	//状态枚举
	enum
	{
		eSInit = 0,		//初始状态
		eSInvite,		//已经收到Invite并进行了正常应答
		eSAck,			//已经收到Ack
		eSBye,			//已经结束会话
	};

	//当前状态
	int m_nState;

	//创建时间
	DWORD m_dwBorn;

	//最后一次收到Rtcp包的时间
	DWORD m_dwLastRtcp;

	//Rtp信息
	DWORD m_dwSSRC;//Rtp头中的SSRC
	int m_nPT;		//Rtp头中的PT


	//回调函数和环境变量
	CT_RtpNotify m_pfnRtp;//Rtp发送开始/停止通知回调函数和环境变量
	CT_Addr m_pfnAddr;//tcp被动传输时，地址获取回调函数
	LPVOID m_lpContext;

	//网络收发对象
	std::shared_ptr<CUdpM> m_udpRtp;
	std::shared_ptr<CUdpM> m_udpRtcp;

	//Rtsp 码流接收对象
	//std::shared_ptr<CRtspClient> m_rtsp;

	std::shared_ptr<CMeadiaPaser> m_pMeadia;


	//rtcp对象
	CRtcp m_Rtcp;

	//rtcp包缓冲区
	Tool::TBuff<BYTE> m_buffRtcp;

	//BYE命令字符串
	std::string m_strBye;

	//上一个命令的CSeq
	std::string m_strCSeq;

	//Rtp输出使用端口计数
	static int s_nStartPort;

	//主动tcp传输对象
	std::shared_ptr<CTcpClient> m_pTcpActive;

	//被动tcp传输对象
	std::shared_ptr<CTcpClientBase> m_pTcpPassive;

	//远端地址
	std::string m_strRemoteIP;
	int m_nRemotePort;

	//tcp发送缓冲区
	Tool::TBuff<char> m_buffSend;

public:
	//本会话ID
	std::string m_strCallID;

	//本地绑定地址
	std::string m_strLocalIP;
	int			m_nRtpPort;

	bool		m_bStart;

	int			m_nRecvRtpPort;

protected:
	bool OnInvite(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp);
	bool OnAck(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd);
	bool OnBye(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd);	

	bool CheckSdp(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp);

	//应答命令
	bool Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent ="" );

	//接收到数据的回调函数，返回已经处理的数据长度（说明：已经被处理的数据，将在被对象内部删除）
	static int ActiveDataCB (
		std::shared_ptr<CContextBase> pContext,		//环境变量
		Tool::TBuff<char> &Data							//接收到的数据缓冲
		);

	static int PassiveDataCB(
		std::shared_ptr<CContextBase> pContext,		//环境变量
		Tool::TBuff<char> &Data,					//接收到的数据缓冲
		std::weak_ptr<CContextBase> pTcp			//接收者本身的指针
		);

	static int Rfc4571(char *pData, size_t nLen);
};
