#include "stdafx.h"
#include "Catalog.h"
#include "Sip/Counter.h"
#include "UdpM_G.h"

#define Log LogN(4001)

CDeviceInfo::CDeviceInfo()
{
}

CDeviceInfo::~CDeviceInfo()
{
}
bool CDeviceInfo::ResponseOK(Sip::CMsg* pCmd)
{
	std::string strCallId = pCmd->GetHead("Call-ID");
	if (strCallId == m_strCurCallID && Sip::eCmdMessageR == pCmd->m_nType)
	{
		//���豸Ŀ¼�ϱ���Ӧ��
		Log(Tool::Debug, "���յ�CDeviceInfo Response OK\n");
		return true;
	}

	return false;
}

void CDeviceInfo::SendReport()
{
	std::string strCallID, strFromTag;

	Sip::CMsgMsg msg;

	if (m_msgSub.Str().size() > 0)
	{
		msg.SetParam(m_strServerId, m_strLocalId, m_strLocalIP, m_nLocalPort,
			CCounter::GetInstanse().Get("DeviceInfoR"), m_msgSub.Str()[0], strCallID, strFromTag);
		msg.Make();
		strCallID = msg.GetHead("Call-ID");
		strFromTag = msg.m_From.strTag;

		m_strCurCallID = strCallID;

		std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
		if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
		{
			Log(Tool::Error, "<%s>�����豸��Ϣ��ѯ�ϱ���Ϣʧ��", "");
		}
		else
		{
			Log(Tool::Info, "<%s>�����豸��Ϣ��ѯ�ϱ���Ϣ�ɹ�", "");
		}
	}
}

CCatalog::CCatalog()
{
	m_bNeedSend = true;
	m_running = false;
}

CCatalog::~CCatalog()
{
}

int g_Count2 = 0;
bool CCatalog::ResponseOK(Sip::CMsg* pCmd)
{
	std::string strCallId = pCmd->GetHead("Call-ID");
	if (strCallId == m_strCurCallID && Sip::eCmdMessageR == pCmd->m_nType)
	{
		//���豸Ŀ¼�ϱ���Ӧ��
		Log(Tool::Debug, "���յ�ResponseOK��[%d]\n", g_Count2);
		m_bNeedSend = true;
		return true;
	}

	return false;
}

#define TIMEOUT 5
//Ŀ¼��ѯ����Ĵ����߳�
void CCatalog::Entry()
{
	m_running = true;
	std::string strCallID, strFromTag;

	nTime = time(NULL);
	for (size_t i = 0; i < m_msgSub.Str().size(); )
	{
		if (m_bNeedSend)
		{
			m_bNeedSend = false;
			if (time(NULL) > nTime + TIMEOUT)
			{
				Log(Tool::Error, "����Ŀ¼��Ӧ��ʱ\n");
				break;
			}
			Sip::CMsgMsg msg;
			msg.SetParam(m_strServerId, m_strLocalId, m_strLocalIP, m_nLocalPort,
				CCounter::GetInstanse().Get("CatalogR"), m_msgSub.Str()[i], strCallID, strFromTag);
			msg.Make();
			strCallID = msg.GetHead("Call-ID");
			strFromTag = msg.m_From.strTag;

			m_strCurCallID = strCallID;

			std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
			if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
			{
				m_bNeedSend = true;
				Log(Tool::Error, "<%s>���͵�<%d>��Ŀ¼�ϱ���Ϣʧ�ܣ�", "", i);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			else
			{
				//m_bNeedSend = false;
				Log(Tool::Info, "<%s>���͵�<%d>��Ŀ¼�ϱ���Ϣ�ɹ� -- ��<%d>��", "", i, ++g_Count2);
				//++g_Count2;
				++i;
			}
			nTime = time(NULL);
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
	Log(Tool::Info, "�����͵�<%d>��Ŀ¼�ϱ���Ϣ�ɹ���\n", g_Count2);
	Log(Tool::Info, "Ŀ¼�ϱ��߳̽����˳�\n");
	m_running = false;
}
