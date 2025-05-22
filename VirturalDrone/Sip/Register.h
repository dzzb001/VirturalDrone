#pragma once
#include "MsgParser.h"
#include <atomic>
#include <set>

namespace Sip
{

typedef void            *LPVOID; 
typedef unsigned long   DWORD;
//注册状态改变回调函数定义
typedef void (*CT_Register)(LPVOID lpContext,	//环境变量
							bool bRegister		//true：上线 false下线
							);

class CRegister
{
public:
	CRegister(void);

	~CRegister(void);

	//注册注册状态改变回调函数
	void RegCallBack(CT_Register pfn, LPVOID lpContext);

	//设置参数
	void SetParam(
		const std::string &strLocalIp,			//本地IP
		int	nLocalPort,							//本地端口
		const std::string &strServerIp,			//服务器的IP
		int	nServerPort,						//账户PORT
		const std::string &strServerID,			//服务器的ID
		const std::string &strID,				//账户ID
		const std::string &strPassword,			//密码
		int nExpire,							//过期时间，单位秒
		int nHeartInterval,						//心跳间隔，单位秒
		int nHeartCount							//失去心跳几次认为断开
		);

	//输入命令
	bool CmdIn(CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//处理函数
	void Process();

	//注销，已注销成功返回true，否则返回false
	bool UnregisterQ();

	//是否注册成功
	bool IsRegisted();

	//获取服务器的信息
	std::string & ServerID(){return m_strServerID;}
	std::string & ServerIP(){return m_strServerIP;}
	int ServerPort(){return m_nServerPort;}
	std::string & LocalID(){return m_strID;}
	std::string & LocalIP(){return m_strLocalIP;}
	int LocalPort(){return m_nLocalPort;}


protected:

	//
	//信息
	//

	std::string m_strServerID;				//服务器ID
	std::string m_strServerIP;				//服务器IP
	int m_nServerPort;						//服务器端口
	std::string m_strID;					//账户ID	
	std::string m_strLocalIP;				//本地IP
	int m_nLocalPort;						//本地端口
	std::string m_strPassword;				//密码
	int m_nExpire;							//过期时间，单位豪秒
	int m_nHeartInterval;					//心跳间隔，单位豪秒
	int m_nHeartCount;						//失去心跳几次认为断开

	//
	//状态
	//

	//状态枚举值
	enum
	{
		eSInit = 0,			//初始
		eSRegister,			//已发送注册
		eSHeart,			//已发送心跳
		eSUnregister,			//已发送注销
	};	
	int m_nState;						//当前状态
	DWORD m_dwTime;						//当前状态的时间
	bool m_bAuth;						//上次注册是否已经带了注册信息
	DWORD m_dwRegOK;					//最后一次注册成功的时间
	std::string m_strCallID;			//最后一次的命令的CallID
	DWORD m_nFaildCount;				//等待命令应答失败计数
	std::string m_strRegCallID;			//当前注册的Call-ID
	std::string m_strRegFromTag;		//当前注册的Tag

	//注册状态改变回调函数和环境变量
	CT_Register m_pfn;
	LPVOID m_lpContext;

	static std::atomic<int> m_nRegisterCount;


protected:

	//各状态的处理函数
	void OnInit();
	void OnRegister();
	void OnHeart();
	void OnUnRegister();

	//各种输入消息的处理函数
	void OnRegisterR(CMsgRBase *pCmd, CSubMsg *pSubMsg);
	void OnHeartR(CMsgRBase *pCmd, CSubMsg *pSubMsg);
	
	//设置当前工作状态
	void SetState(int nState);

	//设置注册状态
	void SetRegister(bool bRegister);

	//发送注册消息
	void Register(
		int nExpire,
		const std::string &strNonce = "", 
		const std::string &strRealm = "",
		const std::string &strFromTag = "",
		const std::string &strCallID = "",
		const std::string &strOpaque = ""
		);

	//发送心跳消息
	void Heart();

};

}
