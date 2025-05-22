#pragma once

#include "Sip/SipMsg.h"
#include "Sip/SubMsg.h"

//ͨ�������
class CChannel
{
public:

	//����
	CChannel(
		const std::string &strDevID,	//ͨ���������豸ID
		const std::string &strID,		//ͨ����ID
		const std::string &strName		//ͨ��������
		);

	//����
	virtual ~CChannel(void);

	//��������
	virtual void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//��������
	virtual void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd, bool &bUsed);

	//�û���������
	virtual void UserCmdIn(const std::string &strType, std::vector<std::string> &vecParam);

	////����Ƿ��н�ʬ�ͻ��ˣ������������
	virtual void CheckClient();

	//�ر�ͨ�����ѹرճɹ�����true�����򷵻�false
	virtual bool Close() = 0;

	//��ȡID
	std::string &ID(){return m_strID;}

	//��ȡ����
	std::string &Name(){return m_strName;}

	//���ò���
	void SetParam(std::string strServerId, std::string strServerIp, int nServerPort,std::string strLocalIP, int nLocalPort);

protected:


	//�����豸ID
	std::string m_strDevID;

	//ͨ��ID
	std::string m_strID;

	//ͨ������
	std::string m_strName;

	std::string m_strServerID;				//������ID
	std::string m_strServerIP;				//������IP
	int m_nServerPort;						//�������˿�
	std::string m_strLocalIP;				//����IP
	int m_nLocalPort;						//���ض˿�
};
