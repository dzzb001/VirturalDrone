#pragma once
#include "Sip/MsgParser.h"
#include <string>
#include "Client.h"
#include "Tool/TBuff.h"
#include "Tool/TRingBuff.h"
#ifdef _WIN32
//#include "Media/Media.h"  
#endif
#include "Channel.h"
#include "Tool/XMutex.h"


//多媒体通道
class CChannelM : public CChannel
{
public:

#if 1
//tcp被动传输时，远端地址输出，并获取本地服务端口
typedef void(*CT_Addr)(
	LPVOID lpContext,							//输入，环境变量
	const std::string &strCHID,					//输入，通道ID
	const std::string &strCallID,				//输入，会话ID
	const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
	int nRemotePort							//输入，远端接收Rtp的端口
	);

	//构造
	CChannelM(
		const std::string &strDevID,	//通道所属的设备ID
		const std::string &strID,		//通道的ID
		const std::string &strName,		//通道的名称
		const std::string &strFile,		//视频源文件路径名
		const std::string &strPayload	//视频源文件中rtp负载的类型，比如H264/pcap、PS/pcap、H264/TS
		);

	//析构
	~CChannelM(void);

	//注册回调函数
	void RegCB(CT_Addr pfn, LPVOID lpContext)
	{
		m_pfnAddr = pfn;
		m_lpContext = lpContext;
	}

	//命令输入
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//关闭通道，已关闭成功返回true，否则返回false
	bool Close();

	//用户命令输入
	void UserCmdIn(const std::string &strID,	//通道ID
		const std::string &strType,				//时间类型，比如Alarm
		std::vector<std::string> &vecParam		//参数列表
		);

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID);

protected:

	//TCP被动传输视频时，地址回调函数
	CT_Addr m_pfnAddr;
	LPVOID m_lpContext;

	//数据源Rtp负载类型，比如H264、PS
	std::string m_strPayload;

#if 0
	//媒体源对象
	Media::CMedia m_Media;
#endif
	//正在发送数据的客户端列表及访问保护
	std::vector<CClient*> m_vecDst;
	XMutex m_lockDst;

	//客户端列表<CallID, 客户端对象>定义
	typedef std::unordered_map<std::string, CClient*> ClientMap;

	//所有的当前客户端列表<CallID, 客户端对象指针>
	ClientMap m_mapClient;

	//<Subject, CallID>列表
	std::unordered_map<std::string, std::string> m_mapSubject;

protected:
	
	//执行异步Sip命令
	void ExeCmd(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//删除一个客户端
	void Delete(std::string strCallID);

	//发送Rtp通知的处理回调函数
	static void RtpNotifyCB(LPVOID lpContext,
		void *pClient,
		bool bStart
		);

	//Rtp数据处理函数
	static void RtpCB(LPVOID lpContext,	//环境变量
		char *pRtp,						//Rtp包
		int nLen,						//Rtp包长度
		int nPayloadLen
		);

	//应答命令
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent="");

	//解析文件类型字符串，形如：PS/pcap
	void SpliteStr(const std::string str, std::vector<std::string> &vec); 

	static void AddrCB(
		LPVOID lpContext,							//输入，环境变量
		const std::string &strCallID,				//输入，会话ID
		const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
		int nRemotePort							//输入，远端接收Rtp的端口
	);
#endif
};
