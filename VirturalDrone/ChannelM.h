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


//��ý��ͨ��
class CChannelM : public CChannel
{
public:

#if 1
//tcp��������ʱ��Զ�˵�ַ���������ȡ���ط���˿�
typedef void(*CT_Addr)(
	LPVOID lpContext,							//���룬��������
	const std::string &strCHID,					//���룬ͨ��ID
	const std::string &strCallID,				//���룬�ỰID
	const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
	int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);

	//����
	CChannelM(
		const std::string &strDevID,	//ͨ���������豸ID
		const std::string &strID,		//ͨ����ID
		const std::string &strName,		//ͨ��������
		const std::string &strFile,		//��ƵԴ�ļ�·����
		const std::string &strPayload	//��ƵԴ�ļ���rtp���ص����ͣ�����H264/pcap��PS/pcap��H264/TS
		);

	//����
	~CChannelM(void);

	//ע��ص�����
	void RegCB(CT_Addr pfn, LPVOID lpContext)
	{
		m_pfnAddr = pfn;
		m_lpContext = lpContext;
	}

	//��������
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//�ر�ͨ�����ѹرճɹ�����true�����򷵻�false
	bool Close();

	//�û���������
	void UserCmdIn(const std::string &strID,	//ͨ��ID
		const std::string &strType,				//ʱ�����ͣ�����Alarm
		std::vector<std::string> &vecParam		//�����б�
		);

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID);

protected:

	//TCP����������Ƶʱ����ַ�ص�����
	CT_Addr m_pfnAddr;
	LPVOID m_lpContext;

	//����ԴRtp�������ͣ�����H264��PS
	std::string m_strPayload;

#if 0
	//ý��Դ����
	Media::CMedia m_Media;
#endif
	//���ڷ������ݵĿͻ����б����ʱ���
	std::vector<CClient*> m_vecDst;
	XMutex m_lockDst;

	//�ͻ����б�<CallID, �ͻ��˶���>����
	typedef std::unordered_map<std::string, CClient*> ClientMap;

	//���еĵ�ǰ�ͻ����б�<CallID, �ͻ��˶���ָ��>
	ClientMap m_mapClient;

	//<Subject, CallID>�б�
	std::unordered_map<std::string, std::string> m_mapSubject;

protected:
	
	//ִ���첽Sip����
	void ExeCmd(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd);

	//ɾ��һ���ͻ���
	void Delete(std::string strCallID);

	//����Rtp֪ͨ�Ĵ���ص�����
	static void RtpNotifyCB(LPVOID lpContext,
		void *pClient,
		bool bStart
		);

	//Rtp���ݴ�����
	static void RtpCB(LPVOID lpContext,	//��������
		char *pRtp,						//Rtp��
		int nLen,						//Rtp������
		int nPayloadLen
		);

	//Ӧ������
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent="");

	//�����ļ������ַ��������磺PS/pcap
	void SpliteStr(const std::string str, std::vector<std::string> &vec); 

	static void AddrCB(
		LPVOID lpContext,							//���룬��������
		const std::string &strCallID,				//���룬�ỰID
		const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
		int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);
#endif
};
