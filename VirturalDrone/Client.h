#pragma once
#include "Sip/SipMsg.h"
#include "Sip/SubMsg.h"
#include "UdpM_G.h"

#include "Rtcp.h"

#include "TcpClient.h"
#include "TcpServer.h"
//#include "RtspClient.h"
#include "Media/MeadiaPaser.h"

//Rtp���Ϳ�ʼ/ֹ֪ͣͨ�ص���������
typedef void (*CT_RtpNotify)(LPVOID lpContext,	//��������
						 void *pClient,			//����֪ͨ�Ŀͻ���ָ��
						 bool bStart			//true����ʼ����Rtp��false��ֹͣ����Rtp
						 );

//typedef void(*CT_RtpRecv)(void * lpContext,	//��������
//						  unsigned char *pData,						//����֪ͨ�Ŀͻ���ָ��
//						  unsigned int nSize
//	);

class CClient : public CContextBase
{
public:

//tcp��������ʱ��Զ�˵�ַ���
typedef void(*CT_Addr)(
	LPVOID lpContext,							//���룬��������
	const std::string &strCallID,				//���룬�ỰID
	const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
	int nRemotePort								//���룬Զ�˽���Rtp�Ķ˿�
	);

	CClient(
		const std::string &strDevID,	//�����豸��ID
		const std::string &strPayload	//Rtp�ĸ�����������
		);

	CClient::CClient();

	~CClient(void);

	//���ò���
	void SetParam(
		const std::string &strDevID,	//�����豸��ID
		const std::string &strPayload	//Rtp�ĸ�����������
	);

	void SetParam(
		const std::string &strDevID,	//�����豸��ID
		const std::string &strPayload,	//Rtp�ĸ�����������
		const std::string &strRtspUrl	//ǰ����ƵԴ�豸url
	);
	//ע��Rtp����֪ͨ�ص�����
	void RegCB(CT_RtpNotify pfnRtp, CT_Addr pfnAddr, LPVOID lpContext);

	//��������
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//�ûỰ�Ƿ��Ѿ�����
	bool Dead();

	//Rtp��������
	void RtpIn(char *pBuff, int nLen, int nPayloadLen);

	//�����ر�
	bool Close();

	//�����ڲ��߼�
	void Process() {};

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient);

	//����rtp���󲢳�ʼ��
	bool CreateUdp( int nPort, std::string strLocalIP, std::shared_ptr<CUdpM> &udp, int nType);
	bool DestoryUdp(std::shared_ptr<CUdpM> &udp);

	//rtp ���ݻص�
	static void RtpUdpDataCB(
		std::shared_ptr<void> pContext,	//��������
		const char *pData,							//���յ�������ָ��
		int nLen,									//���յ��������ֽ���
		const std::string &strFromIP,				//����ԴIP
		short nFromPort								//����Դ�˿�
	);

	//rtcp ���ݻص�
	static void RtcpUdpDataCB(
		std::shared_ptr<void> pContext,	//��������
		const char *pData,							//���յ�������ָ��
		int nLen,									//���յ��������ֽ���
		const std::string &strFromIP,				//����ԴIP
		short nFromPort								//����Դ�˿�
	);

	static void RtpDataCB(void* lpContext,	//��������
		unsigned char *pData,			    //����֪ͨ�Ŀͻ���ָ��
		unsigned int nSize
		);

	//����rtsp��ͻ��ķ���rtp����
	//void AttchRtsp(std::shared_ptr<CRtspClient> pRtsp);

	static void tcpStartCallback(
		std::shared_ptr<void> pContext,	//��������
		int nTimer				//��ʱ��ID
	);
protected:

	//�����豸ID
	std::string m_strDevID;


	//��������
	std::string m_strPayload;

	//ǰ����ƵԴ�豸url
	std::string m_strRtspUrl;

	//״̬ö��
	enum
	{
		eSInit = 0,		//��ʼ״̬
		eSInvite,		//�Ѿ��յ�Invite������������Ӧ��
		eSAck,			//�Ѿ��յ�Ack
		eSBye,			//�Ѿ������Ự
	};

	//��ǰ״̬
	int m_nState;

	//����ʱ��
	DWORD m_dwBorn;

	//���һ���յ�Rtcp����ʱ��
	DWORD m_dwLastRtcp;

	//Rtp��Ϣ
	DWORD m_dwSSRC;//Rtpͷ�е�SSRC
	int m_nPT;		//Rtpͷ�е�PT


	//�ص������ͻ�������
	CT_RtpNotify m_pfnRtp;//Rtp���Ϳ�ʼ/ֹ֪ͣͨ�ص������ͻ�������
	CT_Addr m_pfnAddr;//tcp��������ʱ����ַ��ȡ�ص�����
	LPVOID m_lpContext;

	//�����շ�����
	std::shared_ptr<CUdpM> m_udpRtp;
	std::shared_ptr<CUdpM> m_udpRtcp;

	//Rtsp �������ն���
	//std::shared_ptr<CRtspClient> m_rtsp;

	std::shared_ptr<CMeadiaPaser> m_pMeadia;


	//rtcp����
	CRtcp m_Rtcp;

	//rtcp��������
	Tool::TBuff<BYTE> m_buffRtcp;

	//BYE�����ַ���
	std::string m_strBye;

	//��һ�������CSeq
	std::string m_strCSeq;

	//Rtp���ʹ�ö˿ڼ���
	static int s_nStartPort;

	//����tcp�������
	std::shared_ptr<CTcpClient> m_pTcpActive;

	//����tcp�������
	std::shared_ptr<CTcpClientBase> m_pTcpPassive;

	//Զ�˵�ַ
	std::string m_strRemoteIP;
	int m_nRemotePort;

	//tcp���ͻ�����
	Tool::TBuff<char> m_buffSend;

public:
	//���ỰID
	std::string m_strCallID;

	//���ذ󶨵�ַ
	std::string m_strLocalIP;
	int			m_nRtpPort;

	bool		m_bStart;

	int			m_nRecvRtpPort;

protected:
	bool OnInvite(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp);
	bool OnAck(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd);
	bool OnBye(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd);	

	bool CheckSdp(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp);

	//Ӧ������
	bool Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent ="" );

	//���յ����ݵĻص������������Ѿ���������ݳ��ȣ�˵�����Ѿ�����������ݣ����ڱ������ڲ�ɾ����
	static int ActiveDataCB (
		std::shared_ptr<CContextBase> pContext,		//��������
		Tool::TBuff<char> &Data							//���յ������ݻ���
		);

	static int PassiveDataCB(
		std::shared_ptr<CContextBase> pContext,		//��������
		Tool::TBuff<char> &Data,					//���յ������ݻ���
		std::weak_ptr<CContextBase> pTcp			//�����߱����ָ��
		);

	static int Rfc4571(char *pData, size_t nLen);
};
