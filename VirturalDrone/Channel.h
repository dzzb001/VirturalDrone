#pragma once

#include "Sip/SipMsg.h"
#include "Sip/SubMsg.h"

//通道虚基类
class CChannel
{
public:

	//构造
	CChannel(
		const std::string &strDevID,	//通道所属的设备ID
		const std::string &strID,		//通道的ID
		const std::string &strName		//通道的名称
		);

	//析构
	virtual ~CChannel(void);

	//命令输入
	virtual void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//命令输入
	virtual void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd, bool &bUsed);

	//用户命令输入
	virtual void UserCmdIn(const std::string &strType, std::vector<std::string> &vecParam);

	////检测是否有僵尸客户端，清除垃圾码流
	virtual void CheckClient();

	//关闭通道，已关闭成功返回true，否则返回false
	virtual bool Close() = 0;

	//获取ID
	std::string &ID(){return m_strID;}

	//获取名称
	std::string &Name(){return m_strName;}

	//设置参数
	void SetParam(std::string strServerId, std::string strServerIp, int nServerPort,std::string strLocalIP, int nLocalPort);

protected:


	//所属设备ID
	std::string m_strDevID;

	//通道ID
	std::string m_strID;

	//通道名称
	std::string m_strName;

	std::string m_strServerID;				//服务器ID
	std::string m_strServerIP;				//服务器IP
	int m_nServerPort;						//服务器端口
	std::string m_strLocalIP;				//本地IP
	int m_nLocalPort;						//本地端口
};
