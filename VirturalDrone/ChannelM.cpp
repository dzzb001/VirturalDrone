#include "stdafx.h"
#include "ChannelM.h"
#include "Config.h"
#ifdef _WIN32
#include "Tool/UdpY.h"
#endif
#include <algorithm>

#define Log LogN(4002)

#if 1
/*******************************************************************************
*��    ��:	����
*�������:	strDevID		-- ͨ���������豸ID
*			strID			-- ͨ����ID
*			strName			-- ͨ��������
*			strFile			-- ��ƵԴ�ļ�·����
*			strPayload		-- ��ƵԴ�ļ���rtp���ص����ͣ�����H264��PS
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-19	˾����			����		
*******************************************************************************/
CChannelM::CChannelM( const std::string &strDevID, const std::string &strID, 
				   const std::string &strName, const std::string &strFile,
				   const std::string &strPayload)
: CChannel(strDevID, strID, strName)
, m_pfnAddr(nullptr)
, m_lpContext(nullptr)
{
	std::vector<std::string> vec;
	SpliteStr(strPayload, vec);
	if (vec.size() == 0)
	{
		vec.push_back("PS");
		vec.push_back("pcap");
	}
	else if (vec.size() == 1)
	{
		vec.push_back("pcap");
	}
	if (vec[0] != "PS" && vec[0] != "H264")
	{
		vec[0] = "PS";
	}
	if (vec[1] != "pcap" && vec[1] != "TS")
	{
		vec[1] = "pcap";
	}
	m_strPayload = vec[0];
#if 0
	std::unordered_map<std::string, Media::CMedia::FileT> map;
	map["pcap"] = Media::CMedia::ePcap;
	map["TS"] = Media::CMedia::eTs;
	m_Media.Set(strFile, map[vec[1]], RtpCB, this);
#endif
}

//����
CChannelM::~CChannelM(void)
{
	Log("[%s]", __FUNCTION__);
	Close();
}

//��������
void CChannelM::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
#if 1
	if (pCmd->m_To.strID != m_strID)
	{
		Log("[%s] Invite �����ToID<%s> != ��ͨ��ID<%s>��������", __FUNCTION__,
			pCmd->m_To.strID.c_str(), m_strID.c_str());
		return;
	}
	
	//�û��ȼ������û�н����ĻỰ
	std::vector<std::string> vecDeadCallID;
	for (ClientMap::iterator it = m_mapClient.begin(); it != m_mapClient.end(); ++it)
	{
		if(it->second->Dead())
		{
			vecDeadCallID.push_back(it->first);			
		}
	}
	for (size_t i = 0; i < vecDeadCallID.size(); ++i)
	{
		Log("[%s]�Ự<%s>����������ɾ����", __FUNCTION__, vecDeadCallID[i].c_str());
		Delete(vecDeadCallID[i]);
	}
	
	//�ٴ�������
	if(Sip::eCmdInvite == pCmd->m_nType /*&& !m_Media.Start()*/)
	{
		//��Դ�ļ�ʧ��
		Reply(Sip::eRNotFound, pCmd, pSubCmd);
		return;
	}
	ExeCmd(pCmd, pSubCmd);

	////û�пͻ��˹ۿ��˾Ͱ���ƵԴ�ص�
	if (m_mapClient.empty())
	{
		Log("[%s]��ͨ��û�пͻ����ˣ��ر�����Դ��", __FUNCTION__);
		/*m_Media.Stop();*/
	}
#endif
}

void CChannelM::ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID)
{
#if 1
	auto it = m_mapClient.find(strCallID);
	if (m_mapClient.end() != it)
	{
		m_lockDst.Lock();
		it->second->ClientIn(pClient);
		m_lockDst.UnLock();
		return;
	}
	auto p = pClient.lock();
	if (nullptr != p)
	{
		p->QStop();
	}
#endif
}

//�ر�ͨ�����ѹرճɹ�����true�����򷵻�false
bool CChannelM::Close()
{
	//Log("[%s] ID<%s>", __FUNCTION__, m_strID.c_str());
#if 1
	//m_Media.Stop();
	for (size_t i = 0; i < m_vecDst.size(); ++i)
	{
		m_vecDst[i]->Close();
	}
	m_vecDst.clear();
	for (ClientMap::iterator it = m_mapClient.begin(); it != m_mapClient.end(); ++it)
	{
		delete it->second;
	}
	m_mapClient.clear();
	m_mapSubject.clear();
#endif
	return true;
}

/*******************************************************************************
*��    ��:	ִ���첽Sip����
*�������:	mapClient		-- CallID -> ���Ϳͻ��� �б�
*			mapSubject		-- Subject -> CallID �б�
*�������: 	
*�� �� ֵ��	��
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-11	˾����			����		
*******************************************************************************/
void CChannelM::ExeCmd(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
#if 1
	std::string strCallID = pCmd->GetHead("Call-ID");
	ClientMap::iterator it = m_mapClient.find(strCallID);
	if (m_mapClient.end() == it)
	{
		if (Sip::eCmdInvite != pCmd->m_nType)
		{
			Log("[%s]�Ҳ�������������CallID��Ϣ��", __FUNCTION__);
			if (Sip::eCmdBye == pCmd->m_nType)
			{
				Reply(Sip::eRForbidden, pCmd, pSubCmd);
			}
			return;			
		}

		//�����û����Ҫͣ�����ظ��ͻ���
		std::string strSubject = pCmd->GetHead("Subject");
		std::unordered_map<std::string, std::string> ::iterator itSubject = m_mapSubject.find(strSubject);
		if (m_mapSubject.end() != itSubject)
		{
			Delete(itSubject->second);
		}

		//�½�һ���ͻ���
		m_mapSubject[strSubject] = strCallID;
		CClient *pClient = new CClient(m_strDevID, m_strPayload);
		pClient->RegCB(RtpNotifyCB, AddrCB, this);
		m_mapClient[strCallID] = pClient;
		it = m_mapClient.find(strCallID);
	}
	it->second->CmdIn(pCmd, pSubCmd);
	if (it->second->Dead())
	{
		Delete(strCallID);
	}
#endif
}

/*******************************************************************************
*��    ��:	ɾ��һ���ͻ���
*�������:	strCallID		-- Ҫɾ���ͻ��˵�ID
*�������: 	
*�� �� ֵ��	
*����˵��:	�������strCallID����Ҫʹ�����ã��Է�ֹ��ɾ���������ó���
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-15	˾����			����		
*******************************************************************************/
void CChannelM::Delete(std::string strCallID)
{
#if 0
	ClientMap::iterator it = m_mapClient.find(strCallID);
	if (m_mapClient.end() != it)
	{
		m_lockDst.Lock();
		std::vector<CClient*>::iterator itDst = std::find(m_vecDst.begin(), m_vecDst.end(), it->second);
		if (m_vecDst.end() != itDst)
		{
			m_vecDst.erase(itDst);
		}
		m_lockDst.UnLock();

		it->second->Close();
		delete it->second;
		m_mapClient.erase(it);
	}
	for (std::unordered_map<std::string, std::string>::iterator itSubject = m_mapSubject.begin();
		itSubject != m_mapSubject.end(); itSubject++)
	{
		if (itSubject->second == strCallID)
		{
			m_mapSubject.erase(itSubject);
			break;
		}
	}
#endif
}


void CChannelM::RtpNotifyCB(LPVOID lpContext, void *pClient, bool bStart )
{
#if 0
	CChannelM *pThis = (CChannelM*)lpContext;
	pThis->m_lockDst.Lock();
	if (bStart)
	{
		pThis->m_vecDst.push_back((CClient*)pClient);
	}
	else
	{
		std::vector<CClient*>::iterator itDst = std::find(pThis->m_vecDst.begin(), pThis->m_vecDst.end(), (CClient*)pClient);
		if (pThis->m_vecDst.end() != itDst)
		{
			pThis->m_vecDst.erase(itDst);
		}
	}
	pThis->m_lockDst.UnLock();
#endif
}

/*******************************************************************************
*��    ��:	Rtp���ݴ�����
*�������:	lpContext		-- ��������
*			pRtp			-- Rtp��
*			nLen			-- Rtp������
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-11	˾����			����		
*******************************************************************************/
void CChannelM::RtpCB(LPVOID lpContext, char *pRtp, int nLen, int nPayloadLen )
{
#if 0
	CChannelM *pThis = (CChannelM*)lpContext;
	pThis->m_lockDst.Lock();
	for (size_t i = 0; i < pThis->m_vecDst.size(); ++i)
	{
		pThis->m_vecDst[i]->RtpIn(pRtp, nLen, nPayloadLen);
	}
	pThis->m_lockDst.UnLock();
#endif
}

//Ӧ������
void CChannelM::Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent/* ="" */)
{

#ifdef _WIN32
	Sip::CMsgRBase msg;
	msg.Reply(nReply, (Sip::CMsgBase*)pCmd, strContent);
	msg.Make();
	if (!Tool::CUdpY::GetInstance().Send(msg.Str().c_str(), msg.Str().size()))
	{
		Log("[%s]����Ӧ��ʧ�ܣ�", __FUNCTION__);
	}
#endif
}

/*******************************************************************************
*��    ��:	�����ļ������ַ��������磺PS/pcap
*�������:	str				-- ���������ַ���
*�������: 	vec				-- �����������ַ���
*�� �� ֵ��	
*����˵��:	����"/"�ָ�
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-10-19	˾����			����		
*******************************************************************************/
void CChannelM::SpliteStr(const std::string str, std::vector<std::string> &vec)
{
#if 0
	std::string::size_type nStart = str.find_first_not_of('/');
	while(std::string::npos != nStart)
	{
		std::string::size_type nEnd = str.find_first_of('/', nStart);
		if (std::string::npos == nEnd)
		{
			vec.push_back(str.substr(nStart, str.size()-nStart));
			return;
		}
		vec.push_back(str.substr(nStart, nEnd-nStart));
		nStart = str.find_first_not_of('/', nEnd);
	}
#endif
}

void CChannelM::AddrCB( LPVOID lpContext, const std::string &strCallID, const std::string &strRemoteIP, int nRemotePort)
{
#if 0
	CChannelM *pThis = (CChannelM*)lpContext;
	if (nullptr != pThis->m_pfnAddr)
	{
		pThis->m_pfnAddr(pThis->m_lpContext, pThis->m_strID, strCallID, strRemoteIP, nRemotePort);
	}
#endif
}
#endif
