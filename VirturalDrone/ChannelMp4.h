#pragma once
#include "Channel.h"
#include <string>
#include "Client.h"
#include "Tool/TBuff.h"
#include "Tool/TRingBuff.h"
#include "Tool/XMutex.h"
#include "Media/MeadiaPaser.h"

class CChannelMp4 : public CChannel
{
public:

	//构造
	CChannelMp4(
		const std::string &strDevID,	//通道所属的设备ID
		const std::string &strID,		//通道的ID
		const std::string &strName,		//通道的名称
		const std::string &strRtspUrl,	//视频源URL
		const std::string &strPayload	//视频源文件中rtp负载的类型，比如H264/pcap、PS/pcap、H264/TS
	);
	~CChannelMp4();
	
	//命令输入
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//关闭通道，已关闭成功返回true，否则返回false
	virtual bool Close();

	//用户命令输入
	void UserCmdIn(const std::string &strID,	//通道ID
		const std::string &strType,				//时间类型，比如Alarm
		std::vector<std::string> &vecParam		//参数列表
	) {};

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID); // 暂不用

	//检测是否有僵尸客户端，清除垃圾码流
	void CheckClient();
protected:

	//数据源Rtp负载类型，比如H264、PS
	std::string m_strPayload;

	//前端设备rtsp连接url
	std::string m_strRtspUrl;
	
	//正在发送数据的客户端列表及访问保护
	std::vector<std::shared_ptr<CClient>> m_vecDst;
	XMutex m_lockDst;

	//客户端列表<CallID, 客户端对象>定义
	typedef std::unordered_map<std::string, std::shared_ptr<CClient>> ClientMap;

	//所有的当前客户端列表<CallID, 客户端对象指针>
	ClientMap m_mapClient;

	//<Subject, CallID>列表
	std::unordered_map<std::string, std::string> m_mapSubject;

	//Rtsp 码流接收对象
	//std::shared_ptr<CRtspClient> m_rtsp;

	//读取文件流对象
	std::shared_ptr<CMeadiaPaser> m_pFileStream;

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

	//应答命令
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent = "");

	//解析文件类型字符串，形如：PS/pcap
	void SpliteStr(const std::string str, std::vector<std::string> &vec);

	static void AddrCB(
		LPVOID lpContext,							//输入，环境变量
		const std::string &strCallID,				//输入，会话ID
		const std::string &strRemoteIP,				//输入，远端接收Rtp的IP
		int nRemotePort							//输入，远端接收Rtp的端口
	);
};

