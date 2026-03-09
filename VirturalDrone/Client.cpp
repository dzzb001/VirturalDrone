#include "stdafx.h"
#include "Client.h"
#include "Config.h"
#include <sstream>
#include "MonitorManager.h"

#define Log LogN(3210)

//rtcp���ջ������Ĵ�С
#define RTCP_RECV_BUF_SIZE 1024

//�೤ʱ�����ӽ������ɹ�����Ϊ����,��λ����
#define DEAD_THREASHOLD 60000

//Rtpʹ�õĶ˿���Сֵ�����ֵ
#define RTP_PORT_MIN 20000
#define RTP_PORT_MAX 30000

#pragma pack(1)

//Rtpͷ
struct RtpHeader
{
	unsigned char CC				: 4;		//CSRC�ĸ���
	unsigned char   X				: 1;		//��չ��־λ
	unsigned char   P				: 1;		//����־λ
	unsigned char   version		: 2;		//�汾��

	unsigned char   PT				: 7;		//��������
	unsigned char   M				: 1;		//��־λ(�ɾ���Э��ȷ��)

	unsigned short sn;						//���к�
	unsigned int timestamp;				//ʱ���
	unsigned int SSRC;						//ͬ��Դʶ���ʶ��
};

#pragma pack()

int CClient::s_nStartPort = RTP_PORT_MIN;
CClient::CClient(const std::string &strDevID, const std::string &strPayload)
: m_nState(eSInit)
, m_pfnRtp(nullptr)
, m_pfnAddr(nullptr)
, m_lpContext(nullptr)
, m_buffRtcp(RTCP_RECV_BUF_SIZE)
, m_dwLastRtcp(0)
, m_strDevID(strDevID)
, m_strPayload(strPayload)
, m_nRemotePort(0)
, m_nRecvRtpPort(0)
, m_pTcpActive(nullptr)
, m_pTcpPassive(nullptr)
//, m_bDecoder(false)
{
	m_dwBorn = GetTickCount();
}

CClient::CClient()
	: m_nState(eSInit)
	, m_pfnRtp(nullptr)
	, m_pfnAddr(nullptr)
	, m_lpContext(nullptr)
	, m_buffRtcp(RTCP_RECV_BUF_SIZE)
	, m_dwLastRtcp(0)
	, m_strDevID("")
	, m_strPayload("")
	, m_nRemotePort(0)
	, m_bStart(false)
//	, m_bDecoder(false)
{
	m_dwBorn = GetTickCount();
}


CClient::~CClient(void)
{
	Log(Tool::Debug, "%s", __FUNCTION__);
}

void CClient::SetParam(
	const std::string &strDevID,	//�����豸��ID
	const std::string &strPayload	//Rtp�ĸ�����������
)
{
	m_strDevID = strDevID;
	m_strPayload=strPayload;
}

void CClient::SetParam(
	const std::string &strDevID,	//�����豸��ID
	const std::string &strPayload,	//Rtp�ĸ�����������
	const std::string &strRtspUrl	//ǰ����ƵԴ�豸url
)
{
	m_strDevID = strDevID;
	m_strPayload = strPayload;
	m_strRtspUrl = strRtspUrl;
}

//ע��Rtp����֪ͨ�ص�����
void CClient::RegCB(CT_RtpNotify pfnRtp, CT_Addr pfnAddr, LPVOID lpContext)
{
	m_pfnRtp = pfnRtp;
	m_pfnAddr = pfnAddr;
	m_lpContext = lpContext;
}

//��������
void CClient::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	std::string strCSeq = pCmd->GetHead("CSeq");
	if (strCSeq == m_strCSeq)
	{
		//�ط��������������
		return;
	}
	bool bRet = false;
	switch(pCmd->m_nType)
	{
	case  Sip::eCmdInvite:
		bRet = OnInvite((Sip::CMsgBase*)pCmd, (Sip::CSubMsgSdp*)pSubCmd);
		break;
	case Sip::eCmdAck:
		bRet = OnAck((Sip::CMsgBase*)pCmd, pSubCmd);
		break;
	case Sip::eCmdBye:
		bRet = OnBye((Sip::CMsgBase*)pCmd, pSubCmd);
		break;
	default:
		break;
	}
	if(bRet)
	{
		m_strCSeq = strCSeq;
	}
}

//�ûỰ�Ƿ��Ѿ��ر�
bool CClient::Dead()
{
	return (m_nState == eSBye ||
		((eSAck != m_nState || (m_pTcpActive && !m_pTcpActive->IsConnected())) && GetTickCount() - m_dwBorn > DEAD_THREASHOLD) ||
		(0 != m_dwLastRtcp && GetTickCount() - m_dwLastRtcp > DEAD_THREASHOLD) ||
		(m_pTcpPassive && !m_pTcpPassive->IsConnected())
		);
}

//Rtp��������
void CClient::RtpIn(char *pBuff, int nLen, int nPayloadLen)
{
	/*if (nLen > 1500)
		return;*/
	if (nLen < sizeof(RtpHeader))
	{
		return;
	}
	RtpHeader *pHeader = (RtpHeader*)pBuff;
	//只修改音频PT
	if (pHeader->PT ==  96 || pHeader->PT == 98 || pHeader->PT == 35 || pHeader->PT == 27)
	{
		pHeader->PT = m_nPT;
	}
	pHeader->SSRC = htonl(m_dwSSRC);
	if (nullptr != m_pTcpActive)
	{
		bool b =  m_pTcpActive->IsConnected();
		if (b)
		{
			m_buffSend.clear();
			m_buffSend.resize(2);
			*((unsigned short*)(&m_buffSend[0])) = htons((unsigned short)nLen);
			m_buffSend.append(pBuff, nLen);
			int nSend = m_pTcpActive->Send(&m_buffSend[0], m_buffSend.size());
			//static int n = 0;
			//if (n++ < 10)
			{
				//Log("Send Len[%d],nSend[%d]", m_buffSend.size(), nSend);
			}
		}
		else {
			;//printf("Active is not connected\n");
		}
	}
	else if(nullptr != m_pTcpPassive)
	{
		m_buffSend.clear();
		m_buffSend.resize(2);
		*((unsigned short*)(&m_buffSend[0])) = htons((unsigned short)nLen);
		m_buffSend.append(pBuff, nLen);
		m_pTcpPassive->Send(&m_buffSend[0], m_buffSend.size());
	}
	else
	{
		if (m_udpRtp) {
			//Log(Tool::Debug, "UDP Send to %s:%d, len=%d", m_strRemoteIP.c_str(), m_nRemotePort, nLen);
			m_udpRtp->Send(pBuff, nLen, m_strRemoteIP, m_nRemotePort);
		}
		m_Rtcp.RtpIn((BYTE*)pBuff, nLen, nPayloadLen);
		if (m_Rtcp.GetRtcp(m_buffRtcp, false))
		{
			m_udpRtcp->Send((char *)&m_buffRtcp[0], m_buffRtcp.size(), m_strRemoteIP, m_nRemotePort+1);
			m_buffRtcp.clear();
			OutputDebugString("send udp rtcp paket\n");
			//while (1)
			//{
			//	int nRecv = m_udpRtcp->((char*)&m_buffRtcp[0], RTCP_RECV_BUF_SIZE);
			//	if (nRecv > 0)
			//	{
			//		//Log("���յ�Rtcp��");
			//		m_Rtcp.RtcpIn(&m_buffRtcp[0], nRecv);
			//		continue;
			//	}
			//	break;
			//};
		}
	}
}

//�����ر�
bool CClient::Close()
{
	//����ֹͣrtsp����
	//m_rtsp->RtspStop();
	//��ֹͣ����Rtp
	if ( NULL != m_pfnRtp)
	{
		m_pfnRtp(m_lpContext, this, false);
	}
	DestoryUdp(m_udpRtp);
	//m_udpRtp.Stop();
	if (nullptr != m_pTcpActive)
	{
		m_pTcpActive->QStop();
		m_pTcpActive.reset();
		m_pTcpActive = nullptr;
	}

	if (nullptr != m_pTcpPassive)
	{
		m_pTcpPassive->QStop();
		m_pTcpPassive.reset();
		m_pTcpPassive = nullptr;
	}
	//��ֹͣRtcp
	/*if(m_Rtcp.GetRtcp(m_buffRtcp, true))
	{
		m_udpRtcp.SendInManual(&m_buffRtcp[0], m_buffRtcp.size());
	}*/
	DestoryUdp(m_udpRtcp);
	//m_udpRtcp.Stop();


	//�ٷ���BYE
	if (eSAck == m_nState )
	{
		m_nState = eSBye;
		std::shared_ptr<CUdpM> pUdpM = Tool::CUdpM_G::GetInstance().GetUdpInstance();
		pUdpM->Send(m_strBye.c_str(), m_strBye.size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort());
	}
	return true;
}

void CClient::ClientIn(std::weak_ptr<CTcpClientBase> pClient)
{
	if (nullptr != m_pTcpPassive)
	{
		m_pTcpPassive->QStop();
	}
	m_pTcpPassive = pClient.lock();
	m_pTcpPassive->RegCB(PassiveDataCB, nullptr, nullptr, nullptr);
}

//�յ�INVITE�Ĵ���
bool CClient::OnInvite(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp)
{
	if (eSInit != m_nState)
	{
		Log(Tool::Warning, "[%s] �յ���������IVITE��State=<%d>", __FUNCTION__, m_nState);
		Reply(Sip::eRForbidden, pCmd, (Sip::CSubMsg*)pCmdSdp);
		return false;
	}
	if (!CheckSdp(pCmd, pCmdSdp))
	{
		Reply(Sip::eRNotExist, pCmd, (Sip::CSubMsg*)pCmdSdp);
		return false;
	}
	Config::CConfig &config = Config::CConfig::GetInstance();
	Config::DevNode nodeDev;
	if (!config.GetDevInfo(m_strDevID, nodeDev))
	{
		Reply(Sip::eRServerError, pCmd, (Sip::CSubMsg*)pCmdSdp);
		return false;
	}

	//Reply(Sip::eRTrying, pCmd, (Sip::CSubMsg*)pCmdSdp);
	std::map<std::string, std::string> mapMode = { { "active", "passive" },{ "passive", "active" } };
	std::string strLocalIP = config.GetLocalIP();
	int nRtpPort(0);
	std::string strProtocol = "TCP/RTP/AVP";
	std::string strMode = "passive";//���ص�ģʽ
	std::string strRemoteIP;
	unsigned short nRemotePort = 0;

	if (NULL != pCmdSdp)
	{
		strProtocol = pCmdSdp->m_strProtocol;
		strMode = mapMode[pCmdSdp->m_strMode];
		strRemoteIP = pCmdSdp->m_strIP;
		nRemotePort = pCmdSdp->m_nRtpPort;
	}

	if (strProtocol == "TCP/RTP/AVP")
	{
		OutputDebugString("TCP/RTP/AVP\n");
		if (strMode == "passive")
		{
			if (nullptr != m_pfnAddr && !pCmdSdp->m_strIP.empty())
			{
				m_pfnAddr(m_lpContext, pCmd->GetHead(STR_HEAD_CALLID), pCmdSdp->m_strIP, pCmdSdp->m_nRtpPort);
			}
			nRtpPort = config.GetLocalMeidaPort();
		}
		else
		{
			auto pMonitor = Tool::CMonitorManager::GetInstance().GetMonitor();
			for (nRtpPort = s_nStartPort + 2; nRtpPort != s_nStartPort; nRtpPort += 2)
			{
				m_pTcpActive = CTcpClient::Create<CTcpClient>();
				m_pTcpActive->SetMonitor(pMonitor);
				m_pTcpActive->RegCallback(ActiveDataCB, nullptr, nullptr, nullptr);
				if (m_pTcpActive->PreSocket(strLocalIP, nRtpPort))
				{
					break;
				}
			}
		}
	}
	else
	{
		OutputDebugString("RTP/AVP\n");
		//��ȡ����Rtp��Rtcp�˿�
		for (nRtpPort = s_nStartPort + 2; nRtpPort != s_nStartPort; nRtpPort += 2)
		{
			if (nRtpPort >= RTP_PORT_MAX)
			{
				nRtpPort = RTP_PORT_MIN - 2;
				continue;
			}

			//����rtp udp��������
			if (!CreateUdp(nRtpPort, strLocalIP.c_str(), m_udpRtp, 0)) {
				continue;
			}
			if (!CreateUdp(nRtpPort + 1, strLocalIP.c_str(), m_udpRtcp, 1)) {
				DestoryUdp(m_udpRtp);
				continue;
			}
			m_dwLastRtcp = GetTickCount();
			break;
		}

		m_strLocalIP = strLocalIP;
		m_nRtpPort = nRtpPort;
		//m_rtsp->RtspPlay();
	}
	if (/*nRtpPort == s_nStartPort ||*/ 0 == nRtpPort)
	{
		Log(Tool::Error, "��ȡRtp���Ͷ˿�ʧ�ܣ�Mode<%s> RtpPort<%d> StartPort<%d>", pCmdSdp->m_strMode.c_str(), nRtpPort, s_nStartPort);
		Reply(Sip::eRServerError, pCmd, (Sip::CSubMsg*)pCmdSdp);
		return false;
	}
	if (pCmdSdp->m_strProtocol == "RTP/AVP" || pCmdSdp->m_strMode == "active")
	{
		s_nStartPort = nRtpPort;
	}

	//��������Ӧ��
	//std::unordered_map<std::string, std::string> mapMode = { {"active", "passive"},{"passive", "active"} };
	Sip::CSubMsgSdp msgSubR;
	msgSubR.SetParam(
	m_strDevID,
	config.GetLocalIP(),
    nodeDev.strPassword,
	"sendonly",
	pCmdSdp->m_strProtocol,
	strMode,
	nRtpPort, 
	"Play",
	pCmdSdp->m_strSSRC,
	m_strPayload,
	m_nPT
	);
	msgSubR.Make();
	if (msgSubR.Str().size() < 1)
	{
		DestoryUdp(m_udpRtp);
		DestoryUdp(m_udpRtcp);

		if (nullptr != m_pTcpActive)
		{
			m_pTcpActive->QStop();
			m_pTcpActive.reset();
		}
		if (nullptr != m_pTcpPassive)
		{
			m_pTcpPassive->QStop();
			m_pTcpPassive.reset();
		}
		Reply(Sip::eRServerError, pCmd, (Sip::CSubMsg*)pCmdSdp);
		return false;
	}
	m_strRemoteIP = strRemoteIP;
	m_nRemotePort = nRemotePort;
	m_strCallID = pCmd->GetHead(STR_HEAD_CALLID);
	Reply(Sip::eROK, pCmd, pCmdSdp, msgSubR.Str()[0]);
	m_nState = eSInvite;
	return true;
}

//�յ�ACK�Ĵ���
bool CClient::OnAck(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd)
{
	if (eSInvite != m_nState)
	{
		Log(Tool::Warning, "[%s] �յ���������ACK��State=<%d>", __FUNCTION__, m_nState);
		return false;
	}
	if (nullptr != pSubCmd)
	{
		auto pSdp = (Sip::CSubMsgSdp*)pSubCmd;
		m_strRemoteIP = pSdp->m_strIP;
		m_nRemotePort = pSdp->m_nRtpPort;
	}
	if (nullptr != m_pTcpActive)
	{
		auto pMonitor = Tool::CMonitorManager::GetInstance().GetMonitor();
		pMonitor->AddTimer(1000,m_pThis.lock(), tcpStartCallback);
		//m_pTcpActive->Start(m_strRemoteIP, m_nRemotePort);
	}
	else if( nullptr != pSubCmd && nullptr != m_pfnAddr)
	{
		auto pSdp = (Sip::CSubMsgSdp*)pSubCmd;
		if ("TCP/RTP/AVP" == pSdp->m_strProtocol && "active" == pSdp->m_strMode)
		{
			m_pfnAddr(m_lpContext, pCmd->GetHead(STR_HEAD_CALLID), m_strRemoteIP, m_nRemotePort);
		}
	}
	m_nState = eSAck;
	if (NULL != m_pfnRtp)
	{
		m_pfnRtp(m_lpContext, this, true);
	}
	//������һ��BYE����Ա�����
	Config::CConfig &config = Config::CConfig::GetInstance();
	Sip::CMsgBye msgBye;
	msgBye.SetParam(pCmd->m_From.strID, pCmd->m_From.strTag, pCmd->m_To.strID,
		config.GetLocalIP(), config.GetLocalPort(), pCmd->m_To.strTag, pCmd->m_CSeq.nNo,
		pCmd->GetHead("Call-ID"));
	msgBye.Make();
	m_strBye = msgBye.Str();

	Log(Tool::Debug, "from ID:%s, to ID:%s\n", pCmd->m_From.strID.c_str(), pCmd->m_To.strID.c_str());
	return true;
}

bool CClient::OnBye(Sip::CMsgBase *pCmd, Sip::CSubMsg *pSubCmd)
{
	//��ֹͣ����Rtp
	if (eSAck == m_nState && NULL != m_pfnRtp)
	{
		m_pfnRtp(m_lpContext, this, false);
	}

	m_nState = eSBye;
	//�ڷ��ͳɹ�Ӧ��
	Reply(Sip::eROK, pCmd, pSubCmd);
	return true;
}

bool CClient::CheckSdp(Sip::CMsgBase *pCmd, Sip::CSubMsgSdp *pCmdSdp)
{
	if (NULL == pCmdSdp /*|| pCmdSdp->m_strID != pCmd->m_To.strID*/
		|| pCmdSdp->m_strSSRC.empty() || pCmdSdp->m_strSSRC.at(0) != '0'
		|| std::string::npos == pCmdSdp->m_strType.find("recv")
		|| 0 != pCmdSdp->m_strPlayType.compare("Play")
		|| pCmdSdp->m_strIP.empty() || 0 == pCmdSdp->m_nRtpPort
		)
	{
		Log(Tool::Error, "[%s]Sdp��Ϣ���� - 1 ��SSRC<%s>, Type<%s> PlayType<%s> IP<%s> Port<%d>", __FUNCTION__,
			pCmdSdp->m_strSSRC.c_str(), pCmdSdp->m_strType.c_str(), pCmdSdp->m_strPlayType.c_str(), pCmdSdp->m_strIP.c_str(), pCmdSdp->m_nRtpPort);
		return false;
	}
	m_dwSSRC = atoi(pCmdSdp->m_strSSRC.c_str());

	for (std::unordered_map<int, std::string>::iterator it = pCmdSdp->m_mapRtp.begin();
	it != pCmdSdp->m_mapRtp.end(); ++it)
	{
		if (std::string::npos != it->second.find(m_strPayload))
		{
			m_nPT = it->first;
			return true;
		}
	}
	m_nPT = 96;
	return true;
}

//Ӧ������
bool CClient::Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent/* ="" */)
{
	Sip::CMsgRBase *pMsg = NULL;
	Config::CConfig &config = Config::CConfig::GetInstance();
	if(Sip::eCmdInvite == pCmd->m_nType )
	{
		pMsg = new Sip::CMsgInvitR;
		((Sip::CMsgInvitR*)pMsg)->Reply(
			nReply, (Sip::CMsgBase*)pCmd, config.GetLocalIP(), config.GetLocalPort(), strContent);
	}
	else
	{
		pMsg = new Sip::CMsgRBase;
		pMsg->Reply(nReply, (Sip::CMsgBase*)pCmd, strContent);
	}
	pMsg->Make();

	std::shared_ptr<CUdpM> pUdpM = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdpM->Send(pMsg->Str().c_str(), pMsg->Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
	{
		Log(Tool::Error, "[%s]����Ӧ��ʧ�ܣ�", __FUNCTION__);
		delete pMsg;
		return false;
	}
	delete pMsg;
	return true;

}

//����rtp���󲢳�ʼ��
bool CClient::CreateUdp(int nPort, std::string strLocalIP, std::shared_ptr<CUdpM> &udp, int nType)
{
	udp = CUdpM::Create<CUdpM>();
	auto pMonitor =Tool::CMonitorManager::GetInstance().GetMonitor();

	if (nType) {
		udp->RegDataRecv(CClient::RtcpUdpDataCB, /*std::shared_ptr<void>(this)*/m_pThis.lock());
	}
	else {
		udp->RegDataRecv(CClient::RtpUdpDataCB, m_pThis.lock());
	}

	udp->SetMonitor(pMonitor);
	int nStart = 1024 * 1024 * 40;
	if (!udp->Start(nStart, nStart,false, nPort, strLocalIP))
	{
		std::weak_ptr<CUdpM> p = udp;
		udp.reset();
		CContextBase::WaitDestroy(p);
		return false;
	}
	return true;
}

bool CClient::DestoryUdp(std::shared_ptr<CUdpM> &udp)
{
	if (udp) {
		udp->QStop();
		{
			std::weak_ptr<CUdpM> p = udp;
			udp.reset();
			CContextBase::WaitDestroy(p);
		}
	}
	return true;
}

//rtp ���ݻص�
void CClient::RtpUdpDataCB(std::shared_ptr<void> pContext, const char *pData, int nLen, const std::string &strFromIP, short nFromPort)
{
	CClient *pThis = (CClient*)(pContext.get());
	if (pThis) {
		int nPlayLoadLen=0;
		pThis->RtpIn((char *)pData, nLen, nPlayLoadLen);
	}
}

//rtcp ���ݻص�
void CClient::RtcpUdpDataCB(std::shared_ptr<void> pContext, const char *pData, int nLen, const std::string &strFromIP, short nFromPort)
{
	CClient *pThis = (CClient*)(pContext.get());
	if (pThis) 
	{
		if (nLen > 0)
		{
			Log("���յ�Rtcp��");
			pThis->m_Rtcp.RtcpIn((unsigned char *)pData, nLen);
			pThis->m_dwLastRtcp = GetTickCount(); 
		}
		else {
			Log("���յ�Rtcp 0��");
			pThis->m_dwLastRtcp = GetTickCount();//add at 20231017,����nvr��֧��rtcp,������Ӳ��ж��Ƿ��յ�rtcp��
		}
	}
}

void CClient::RtpDataCB(void*  lpContext,	//��������
	unsigned char *pData,						//����֪ͨ�Ŀͻ���ָ��
	unsigned int nSize
)
{
	CClient *pThis = (CClient*)(lpContext);
	if (pThis) {
		int nPlayLoadLen = 0;
		pThis->RtpIn((char *)pData, nSize, nPlayLoadLen);
	}
}

//����rtsp��ͻ��ķ���rtp����
/*
void CClient::AttchRtsp(std::shared_ptr<CRtspClient> pRtsp)
{
	//return pRtsp->AddRecvClient(strCallID, strRtspRcvIP.c_str(), nRtpPort, strRtspRcvIP.c_str(), nRtcpPort);
	m_rtsp = pRtsp;
}
*/

//���յ����ݵĻص������������Ѿ����������ݳ��ȣ�˵�����Ѿ������������ݣ����ڱ������ڲ�ɾ����
int CClient::ActiveDataCB(
	std::shared_ptr<CContextBase> pContext,		//��������
	Tool::TBuff<char> &Data							//���յ������ݻ���
)
{
	return Rfc4571(&Data[0], Data.size());
	
	
}

int CClient::PassiveDataCB (
	std::shared_ptr<CContextBase> pContext,		//��������
	Tool::TBuff<char> &Data,					//���յ������ݻ���
	std::weak_ptr<CContextBase> pTcp			//�����߱�����ָ��
	)
{
	return Rfc4571(&Data[0], Data.size());
}

int CClient::Rfc4571(char *pData, size_t nLen)
{
	//���
	int nEaten = 0;
	while (nEaten + sizeof(unsigned short) + sizeof(RtpHeader) < nLen)
	{
		unsigned short nRtpLen = htons(*(unsigned short*)(pData + nEaten));
		if (nLen < nEaten + sizeof(unsigned short) + nRtpLen)
		{
			return nEaten;
		}
		const char *pRtp = pData + nEaten + sizeof(unsigned short);
		RtpHeader *pHeader = (RtpHeader*)pRtp;
		if (pHeader->version != 2)//rtp��versionΪ2
		{
			Log("[CRfc4571::ExeInputAsSrc]ʧȥͬ��");
			return nLen;//ʧȥͬ��
		}
		nEaten += nRtpLen + sizeof(unsigned short);
		Log("Rtp -- sn<%d>", htons(pHeader->sn));

	}
	return nEaten;
}

void CClient::tcpStartCallback(
	std::shared_ptr<void> pContext,	//��������
	int nTimer				//��ʱ��ID
)
{
	CClient *pThis = (CClient *)pContext.get();
	if (pThis) {
		if (pThis->m_pTcpActive && !pThis->m_bStart)
			pThis->m_bStart = pThis->m_pTcpActive->Start(pThis->m_strRemoteIP, pThis->m_nRemotePort);
	}

	auto pMonitor = Tool::CMonitorManager::GetInstance().GetMonitor();
	pMonitor->DelTimer(nTimer);
}


