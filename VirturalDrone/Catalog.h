#pragma once

#include "UdpM.h"
#include "Sip/MsgParser.h"

#include "Tool/XMutex.h"
#include "Tool/XThread.h"

class CDeviceInfo
{
public:
	CDeviceInfo();
	~CDeviceInfo();

	bool ResponseOK(Sip::CMsg* pCmd);

	void SendReport();

	void SetParam(std::string serId, std::string strLocId, std::string strLocIP, int nPort)
	{
		m_strServerId = serId;
		m_strLocalId = strLocId;
		m_strLocalIP = strLocIP;
		m_nLocalPort = nPort;
	}

public:
	Sip::CSubMsgDeviceInfoR m_msgSub;

protected:
	bool m_bNeedSend;
	XMutex m_LocksetCallID;

	std::string m_strServerId;
	std::string m_strLocalId;
	std::string m_strLocalIP;
	int m_nLocalPort;

	std::string m_strCurCallID;
};

class CCatalog : public XThread
{
public:
	CCatalog();
	~CCatalog();

	bool ResponseOK(Sip::CMsg* pCmd);
	//处理目录上报线程处理函数
	virtual void Entry();

	void StartReport() {
		Start();
	}
	void Stop() {
		//m_bStop = true;
		Join();
	}
	void SetParam(std::string serId, std::string strLocId, std::string strLocIP, int nPort)
	{
		m_strServerId = serId;
		m_strLocalId = strLocId;
		m_strLocalIP = strLocIP;
		m_nLocalPort = nPort;
	}

	bool ReportComplete() {
		return m_running ? false : true;
	}

public:
	Sip::CSubMsgCatalogR m_msgSub;

protected:

	//当前已经上传了设备个数
	int m_nCount;

	bool m_bNeedSend;
	XMutex m_LocksetCallID;

	std::string m_strServerId;
	std::string m_strLocalId;
	std::string m_strLocalIP;
	int m_nLocalPort;

	bool m_running;
	std::string m_strCurCallID;

	long nTime;

	//bool m_bStop;
};

