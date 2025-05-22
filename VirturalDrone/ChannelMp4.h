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

	//����
	CChannelMp4(
		const std::string &strDevID,	//ͨ���������豸ID
		const std::string &strID,		//ͨ����ID
		const std::string &strName,		//ͨ��������
		const std::string &strRtspUrl,	//��ƵԴURL
		const std::string &strPayload	//��ƵԴ�ļ���rtp���ص����ͣ�����H264/pcap��PS/pcap��H264/TS
	);
	~CChannelMp4();
	
	//��������
	void CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd);

	//�ر�ͨ�����ѹرճɹ�����true�����򷵻�false
	virtual bool Close();

	//�û���������
	void UserCmdIn(const std::string &strID,	//ͨ��ID
		const std::string &strType,				//ʱ�����ͣ�����Alarm
		std::vector<std::string> &vecParam		//�����б�
	) {};

	void ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID); // �ݲ���

	//����Ƿ��н�ʬ�ͻ��ˣ������������
	void CheckClient();
protected:

	//����ԴRtp�������ͣ�����H264��PS
	std::string m_strPayload;

	//ǰ���豸rtsp����url
	std::string m_strRtspUrl;
	
	//���ڷ������ݵĿͻ����б����ʱ���
	std::vector<std::shared_ptr<CClient>> m_vecDst;
	XMutex m_lockDst;

	//�ͻ����б�<CallID, �ͻ��˶���>����
	typedef std::unordered_map<std::string, std::shared_ptr<CClient>> ClientMap;

	//���еĵ�ǰ�ͻ����б�<CallID, �ͻ��˶���ָ��>
	ClientMap m_mapClient;

	//<Subject, CallID>�б�
	std::unordered_map<std::string, std::string> m_mapSubject;

	//Rtsp �������ն���
	//std::shared_ptr<CRtspClient> m_rtsp;

	//��ȡ�ļ�������
	std::shared_ptr<CMeadiaPaser> m_pFileStream;

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

	//Ӧ������
	void Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent = "");

	//�����ļ������ַ��������磺PS/pcap
	void SpliteStr(const std::string str, std::vector<std::string> &vec);

	static void AddrCB(
		LPVOID lpContext,							//���룬��������
		const std::string &strCallID,				//���룬�ỰID
		const std::string &strRemoteIP,				//���룬Զ�˽���Rtp��IP
		int nRemotePort							//���룬Զ�˽���Rtp�Ķ˿�
	);
};

