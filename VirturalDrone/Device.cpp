#include "stdafx.h"
#include "Device.h"

#include "Sip/Counter.h"

#ifdef _WIN32
#include "Tool/UdpY.h"
#endif
//#include "ChannelM.h"
//#include "ChannelA.h"
#include "Config.h"
//#include "ChannelR.h"
#include "UdpM_G.h"
//#include "ChannelRtspHistroy.h"
#include "ChannelMp4.h"

#define Log LogN(4001)

CDevice::CDevice(void)
	: m_pfnAddr(nullptr)
	, m_lpContext(nullptr)
{
	m_pCatalog = NULL;
}

CDevice::~CDevice(void)
{
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		delete it->second;
	}

	for (ChannelMap::iterator it = m_mapID2ChRerocd.begin(); it != m_mapID2ChRerocd.end(); ++it)
	{
		delete it->second;
	}
}

//���ò���
void CDevice::SetParam(const std::string &strLocalIp, int	nLocalPort, const std::string &strServerIp, int	nServerPort, const std::string &strServerID, const std::string &strID,
					   const std::string &strPassword, int nExpire, int nHeartInterval, 
					   int nHeartCount )
{
	m_Register.RegCallBack(RegisterCB, this);
	m_Register.SetParam(strLocalIp, nLocalPort, strServerIp, nServerPort, strServerID, strID, strPassword, nExpire, nHeartInterval, nHeartCount);
}

//���ͨ��
void CDevice::AddCh( const std::string &strType, const std::string &strID, const std::string &strName,
					const std::string &strFile, const std::string &strFileRecord, const std::string &strPayload,
					const std::string &strLocalIp,			//����IP
					int	nLocalPort,							//���ض˿�
					const std::string &strServerIp,			//��������IP
					int	nServerPort,						//�˻�PORT
					const std::string &strServerID			//��������ID 
)
{
	if (m_mapID2Ch.end() != m_mapID2Ch.find(strID))
	{
		Log(Tool::Error, "���ͨ��<%s>ʧ�� -- ͨ���Ѿ�����", strID.c_str());
		return;
	}
	if ("Media" == strType)
	{
		/*CChannelM *pCh = new CChannelM(m_Register.LocalID(), strID, strName, strFile, strPayload);
		pCh->RegCB(AddrCB, this);
		m_mapID2Ch[strID] = pCh;*/
	}
	else if ("Alarm" == strType)
	{
		//CChannelA *pCh = new CChannelA(m_Register.LocalID(), strID, strName);
		//m_mapID2Ch[strID] = pCh;
	}
	else if ("Rtsp" == strType) 
	{
		//CChannelR *pCh = new CChannelR(m_Register.LocalID(), strID, strName, strFile, strPayload);
		//ע��ص�
		//m_mapID2Ch[strID] = pCh;

		//CChannelRtspHistroy *pChHis = new CChannelRtspHistroy(m_Register.LocalID(), strID, strName, strFileRecord, strPayload);
		//pChHis->SetParam(strServerID, strServerIp, nServerPort, strLocalIp, nLocalPort);
		//¼��ط�
		//m_mapID2ChRerocd[strID] = pChHis;
	}
	else if ("Mp4" == strType)
	{
		CChannelMp4 *pCh = new CChannelMp4(m_Register.LocalID(), strID, strName, strFile, strPayload);
		//ע��ص�
		m_mapID2Ch[strID] = pCh;
	}
}

/*******************************************************************************
*��    ��:	��������
*�������:	pCmd			-- sip����
*			pSubCmd			-- Sip������Ϣ���е�������
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-2	�ű�			����		
*******************************************************************************/
void CDevice::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	Log(Tool::Debug, "CDevice::CmdIn <%s>", pCmd->ContentStr().c_str());

	//Log("!!!!!!!!!m_Register.LocalID():%s\npCmd->m_To.strID:%s\npCmd->m_From.strID:%s\npCmd->m_nType:%d\n", m_Register.LocalID(),pCmd->m_To.strID, pCmd->m_From.strID);
	//Log("!!!!!!!!!m_Register.LocalID():%s\npCmd->m_To.strID:%s\npCmd->m_From.strID:%s\npCmd->m_nType:%d\npSubCmd->m_nType:%d\n", m_Register.LocalID().c_str(), pCmd->m_To.strID.c_str(), pCmd->m_From.strID.c_str(), pCmd->m_nType, pSubCmd ? pSubCmd->m_nType : 11111);

	//���豸�������
	if(m_Register.LocalID() == pCmd->m_To.strID || m_Register.LocalID() == pCmd->m_From.strID)
	{
		//Log("dev cmdin step1\n");
		if(Sip::eCmdMessage == pCmd->m_nType && NULL != pSubCmd && Sip::eSubCmdCatalogQ == pSubCmd->m_nType)
		{
			//Log("dev cmdin step2\n");
			OnCatalogQ(pCmd, pSubCmd);
			return;
		}
		else if (Sip::eCmdMessage == pCmd->m_nType && NULL != pSubCmd && Sip::eSubCmdDeviceInfoQ == pSubCmd->m_nType)
		{
			OnDeviceInfoQ(pCmd, pSubCmd);
			return;
		}

		if (m_pCatalog) {
			m_pCatalog->ResponseOK(pCmd);
		}

		if (m_Register.CmdIn(pCmd, pSubCmd))
		{
			return;
		}
	}
	else {
		;// Log("CDevice::CmdIn <123456   %s>", pCmd->ContentStr().c_str());
	}

	//ͨ����ص�����
	ChannelMap::iterator it = m_mapID2Ch.find(pCmd->m_To.strID);
	if (m_mapID2Ch.end() == it)
	{
		it = m_mapID2Ch.find(pCmd->m_From.strID);
	}
	if (m_mapID2Ch.end() == it)
	{
		if(Sip::eCmdInvite == pCmd->m_nType || Sip::eCmdBye == pCmd->m_nType ||
			Sip::eCmdMessage == pCmd->m_nType)
		{
			Reply(Sip::eRNotExist, pCmd, pSubCmd);
		}
		Log(Tool::Warning, "[%s]δ�ҵ����ʵĴ�������Ķ���", __FUNCTION__);
		return;
	}
	if (Sip::eCmdInvite == pCmd->m_nType && !m_Register.IsRegisted())
	{
		Log(Tool::Warning, "[%s]�豸<%s>��ǰ��������״̬����������Ƶ�������",
			__FUNCTION__, m_Register.LocalID().c_str());
		return;
	}

	//�ж��Ƿ���¼����Ƶ����
	bool bRecordMessage = false;
	ChannelMap::iterator itReocrd = m_mapID2ChRerocd.find(pCmd->m_To.strID);
	if (m_mapID2ChRerocd.end() != itReocrd)
	{
		itReocrd->second->CmdIn(pCmd, pSubCmd, bRecordMessage);
	}
	//����ʵʱ��Ƶ�����Ϣ
	if(!bRecordMessage)
		it->second->CmdIn(pCmd, pSubCmd);	
}

/*******************************************************************************
*��    ��:	�û���������Ĵ���
*�������:	strID			-- ͨ��ID
*			strType,		-- ʱ�����ͣ�����Alarm
*			vecParam		-- �����б�
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-19	�ű�			����		
*******************************************************************************/
void CDevice::UserCmdIn(const std::string &strID, const std::string &strType, 
						std::vector<std::string> &vecParam )
{
	ChannelMap::iterator it = m_mapID2Ch.find(strID);
	if (m_mapID2Ch.end() == it)
	{
		Log(Tool::Error, "�����û���������ʧ�� - �Ҳ�����ͨ��<%s>", strID.c_str());
		return;
	}
	it->second->UserCmdIn(strType, vecParam);	
}

//�����ڲ�����
void CDevice::Process()
{
	m_Register.Process();
}

/*******************************************************************************
*��    ��:	ע��
*�������:	
*�������: 	
*�� �� ֵ��	�Ѿ�ע���ɹ�����true�����򷵻�false
*����˵��:	������Ӧ�ñ��ظ����ã�ֱ�����سɹ�
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-7-28	�ű�			����		
*******************************************************************************/
bool CDevice::Unregister()
{
	//Log("[%s] ID<%s>", __FUNCTION__, m_Register.LocalID().c_str());
	//�ر�����ͨ��
	bool bClosed = true;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		if(!it->second->Close())
		{
			bClosed = false;
		}
	}
	if (!bClosed)
	{
		return false;
	}

	//ע�����豸
	return m_Register.UnregisterQ();
}

void CDevice::ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strChID, const std::string &strCallID)
{
#if 0
	auto it = m_mapID2Ch.find(strChID);
	if (m_mapID2Ch.end() != it)
	{
		auto pCh = dynamic_cast<CChannelM*>(it->second);
		if (nullptr != pCh)
		{
			pCh->ClientIn(pClient, strCallID);
			return;
		}	
	}	
	auto p = pClient.lock();
	if (nullptr != p)
	{
		p->QStop();
		return;
	}
#endif
}

//�豸�����ߵĴ���ص�����
void CDevice::RegisterCB(void * lpContext, bool bRegister)
{
	CDevice *pThis = (CDevice*)lpContext;
	Log(Tool::Debug, "[%s] ID<%s>", __FUNCTION__, pThis->m_Register.LocalID().c_str());
	if (!bRegister)
	{
		for (ChannelMap::iterator it = pThis->m_mapID2Ch.begin(); it != pThis->m_mapID2Ch.end(); ++it)
		{
			it->second->Close();
		}
	}
	if (bRegister && Config::CConfig::GetInstance().PushDevice())
	{
		Sip::CMsgMsg msg;
		Sip::CSubMsgCatalogQ subMsg;
		subMsg.m_nSN = 0;
		pThis->ReportCatalog(&msg, &subMsg);
	}
}

#define TIMEOUT 5
int g_Count = 0;
//Ŀ¼��ѯ����Ĵ����߳�
//Ŀ¼��ѯ����Ĵ���
void CDevice::OnCatalogQ(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
	if (NULL == pSubCmd || m_Register.LocalID() != pSubCmd->m_strID)
	{
		Reply(Sip::eRNotExist, pCmd, pSubCmd);
		return;
	}
	//Log("��ʼ����Ŀ¼����������");
	Reply(Sip::eROK, pCmd, pSubCmd);

	ReportCatalog2(pCmd, pSubCmd);
}

//�豸��Ϣ��ѯ����Ĵ���
void CDevice::OnDeviceInfoQ(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd)
{
	if (NULL == pSubCmd || m_Register.LocalID() != pSubCmd->m_strID)
	{
		Reply(Sip::eRNotExist, pCmd, pSubCmd);
		return;
	}
	Log("�豸��Ϣ��ѯ����������");
	Reply(Sip::eROK, pCmd, pSubCmd);

	ReportDeviceInfo(pCmd, pSubCmd);
}


void CDevice::ReportCatalog2(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
#if 1
	//����δ���
	/*if (m_pCatalog && !m_pCatalog->ReportComplete())
		return;*/

	//����ɻ����Ѵ�����

	if (m_pCatalog)
	{
		m_pCatalog->Stop();
		delete m_pCatalog;
		m_pCatalog = NULL;
	}

	m_pCatalog = new CCatalog;
	m_pCatalog->SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort());

	Sip::CatalogVec vecCat;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		Sip::CatalogNode node;
		node.strDeviceID = it->second->ID();
		node.strName = it->second->Name();
		node.strParentID = m_Register.LocalID();
		vecCat.push_back(node);
	}

	m_pCatalog->m_msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	m_pCatalog->m_msgSub.Make();

	m_pCatalog->StartReport();
#endif
	return;
}
//�ϱ�Ŀ¼
void CDevice::ReportCatalog(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
	Sip::CatalogVec vecCat;
	for (ChannelMap::iterator it = m_mapID2Ch.begin(); it != m_mapID2Ch.end(); ++it)
	{
		Sip::CatalogNode node;
		node.strDeviceID = it->second->ID();
		node.strName = it->second->Name();
		node.strParentID = m_Register.LocalID();
		vecCat.push_back(node);
	}
	Sip::CSubMsgCatalogR msgSub;
	msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	msgSub.Make();
	std::string strCallID, strFromTag;
	//Log("��ʼ�ϱ�Ŀ¼,<%d> Ŀ¼����<%d>", pSubCmd->m_nSN, msgSub.Str().size());
	for (size_t i = 0; i < msgSub.Str().size(); )
	{
		//Log("��ʼ����<%d>��Ŀ¼", i);
		Sip::CMsgMsg msg;
		msg.SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort(), 
			CCounter::GetInstanse().Get("CatalogR"), msgSub.Str()[i], strCallID, strFromTag);
		msg.Make();
		strCallID = msg.GetHead("Call-ID");
		strFromTag = msg.m_From.strTag;

		std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
		if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
		{
			Log(Tool::Error, "<%s>���͵�<%d>��Ŀ¼�ϱ���Ϣʧ�ܣ�", pCmd->m_To.strID.c_str(), i);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			Log(Tool::Info, "<%s>���͵�<%d>��Ŀ¼�ϱ���Ϣ�ɹ� -- ��<%d>��", pCmd->m_To.strID.c_str(), i, ++g_Count);
			m_setCallID.insert(strCallID);
			++i;
		}
		//Tool::CCommon::m_sleep(50);
	}
}

void CDevice::ReportDeviceInfo(Sip::CMsg* pCmd, Sip::CSubMsg* pSubCmd)
{
	m_deviceInfo.SetParam(m_Register.ServerID(), m_Register.LocalID(), m_Register.LocalIP(), m_Register.LocalPort());

	Sip::CatalogVec vecCat;

	m_deviceInfo.m_msgSub.SetParam(pSubCmd->m_nSN, m_Register.LocalID(), vecCat);
	m_deviceInfo.m_msgSub.Make();

	m_deviceInfo.SendReport();
}

//Ӧ������
void CDevice::Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent/* ="" */)
{
	Sip::CMsgRBase msg;
	msg.Reply(nReply, (Sip::CMsgBase*)pCmd, strContent);
	msg.Make();

	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
	{
		Log(Tool::Error, "����Ӧ��ʧ�ܣ�");
	}
}

void CDevice::AddrCB( void * lpContext, const std::string &strCHID, const std::string &strCallID, const std::string &strRemoteIP,
	int nRemotePort)
{
	CDevice *pThis = (CDevice*)lpContext;
	if (nullptr != pThis->m_pfnAddr)
	{
		pThis->m_pfnAddr(pThis->m_lpContext, pThis->m_Register.LocalID(), strCHID, strCallID, strRemoteIP, nRemotePort);
	}
}

void CDevice::CheckClient()
{
	ChannelMap::iterator itor = m_mapID2Ch.begin();
	for (; itor != m_mapID2Ch.end(); itor++)
	{
		itor->second->CheckClient();
	}
}
