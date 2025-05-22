#include "stdafx.h"
#include "Register.h"
#include <sstream>
#include "Counter.h"
#include "../UdpM_G.h"

//ע�ᡢע������Ӧ��ĳ�ʱʱ�䣬��λ����
#define TIMEOUT_REG 60000

//�˳�ʱ��ע������Ӧ��ĳ�ʱ�ȼ䣬��λ����
#define TIMEOUT_UNREG 5000

//ע��ʱЧ����Сֵ����λ��
#define EXPIRES_MIN 600

//��ǰ����ʱ������ע�ᣬ��λ����
#define REREGISTER_OFFSET 300000

#define Log LogN(6111)

//std::atomic<int> Sip::CRegister::m_nRegisterCount = 1;
std::atomic<int> Sip::CRegister::m_nRegisterCount(1);

namespace Sip
{
CRegister::CRegister(void)
: m_nHeartCount(3)
, m_nHeartInterval(60000)
, m_nExpire(3600000)
, m_nState(eSInit)
, m_dwTime(0)
, m_bAuth(false)
, m_dwRegOK(0)
, m_nFaildCount(0)
, m_pfn(NULL)
, m_lpContext(NULL)
{
}

CRegister::~CRegister(void)
{
}

//ע��ע��״̬�ı�ص�����
void CRegister::RegCallBack(CT_Register pfn, LPVOID lpContext)
{
	m_pfn = pfn;
	m_lpContext = lpContext;
}

/*******************************************************************************
*��    ��:	���ò���
*�������:	strID				-- �˻�ID
*			strPassword			-- ����
*			nExpire				-- ����ʱ�䣬��λ��
*			nHeartInterval		-- �����������λ��
*			nHeartCount			-- ʧȥ����������Ϊ�Ͽ�
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-7-28	�ű�			����		
*******************************************************************************/
void CRegister::SetParam(const std::string &strLocalIp, int	nLocalPort,
						 const std::string &strServerIp,int	nServerPort, const std::string &strServerID,
						 const std::string &strID,
						 const std::string &strPassword, int nExpire, int nHeartInterval, 
						 int nHeartCount )
{
	if (nExpire < EXPIRES_MIN)
	{
		Log(Tool::Info, "[%s]ע�����Чʱ��<%d>�� < %d�룬��������Ϊ%d�룡",
			__FUNCTION__, nExpire, EXPIRES_MIN, EXPIRES_MIN);
		nExpire = /*1800*/600;
	}
	m_strLocalIP = strLocalIp;
	m_nLocalPort = nLocalPort;
	m_strServerIP = strServerIp;
	m_nServerPort = nServerPort;
	m_strServerID = strServerID;
	m_strID = strID;
	m_strPassword = strPassword;
	m_nExpire = nExpire*1000;
	m_nHeartInterval = nHeartInterval*1000;
	m_nHeartCount = nHeartCount;
}

//��������
bool CRegister::CmdIn(CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	if (m_strCallID != pCmd->GetHead("Call-ID"))
	{
		Log(Tool::Warning, "[%s]CallID��ƥ��<%s> != <%s>���� Reg �������",
			__FUNCTION__, m_strCallID.c_str(), pCmd->GetHead("Call-ID").c_str());
		return false;
	}
	switch(pCmd->m_nType)
	{
	case eCmdRegisterR:		OnRegisterR((CMsgRBase*)pCmd, pSubCmd);		break;
	case eCmdMessageR:		OnHeartR((CMsgRBase*)pCmd, pSubCmd);		break;	
	default:
		Log(Tool::Warning, "[%s]�յ�����������������<%d>", __FUNCTION__, pCmd->m_nType);
		break;;
	}
	return true;
}

//������
void CRegister::Process()
{
	switch(m_nState)
	{
	case eSInit:			OnInit();			break;
	case eSRegister:		OnRegister();		break;
	case eSHeart:			OnHeart();			break;
	case eSUnregister:		OnUnRegister();		break;
	default: break;
	}
	if (0 != m_dwRegOK && GetTickCount() - m_dwRegOK > (DWORD)m_nExpire)
	{
		SetRegister(false);	
	}
}

//ע������ע���ɹ�����true�����򷵻�false
bool CRegister::UnregisterQ()
{
	if (eSInit == m_nState || 0 == m_dwRegOK)
	{
		return true;
	}
	if (eSUnregister == m_nState)
	{
		if (GetTickCount() - m_dwTime >= TIMEOUT_UNREG)
		{
			//�Ѿ����͹�ע����Ϣ���ҳ�ʱ��
			return true;
		}
		return false;
	}

	//����ע������
	Register(0, "", "", m_strRegFromTag, m_strRegCallID);
	return false;
}

bool CRegister::IsRegisted()
{
	return 0 != m_dwRegOK;	
}

//��ʼ״̬��ֱ�ӷ�ע����Ϣ
void CRegister::OnInit()
{
	Register(m_nExpire/1000);
}

//ע��״̬�����ע��Ӧ����û�г�ʱ
void CRegister::OnRegister()
{
	if (GetTickCount() - m_dwTime >= TIMEOUT_REG)
	{
		//��ʱ�ˣ��ط�
		Log(Tool::Warning, "[%s]��<%d>�εȴ�ע��Ӧ��ʱ��", __FUNCTION__, m_nFaildCount);
		Register(m_nExpire/1000);
	}
}

//����״̬���������Ӧ����û�г�ʱ
void CRegister::OnHeart()
{
	//����Ƿ�������ע���ʱ����
	if (GetTickCount() - m_dwRegOK > DWORD(m_nExpire - REREGISTER_OFFSET))
	{
		//��ǰ5��������ע��
		Log(Tool::Info, "[%s]ע�ἴ�����ڣ�����ע�ᣡ", __FUNCTION__);
		Register(m_nExpire/1000, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	if (GetTickCount() - m_dwTime < (DWORD)m_nHeartInterval)
	{
		//��û�е��������ʱ��
		return;
	}
	if (m_nFaildCount != 0)
	{
		Log(Tool::Warning, "[%s]��<%d>�εȴ�����Ӧ��ʱ��", __FUNCTION__, m_nFaildCount);
	}
	if (m_nFaildCount >= (DWORD)m_nHeartCount)
	{
		Log(Tool::Warning, "[%s]����ʧ�ܴ���<%d>�ﵽ��ֵ<%d>����ע������ע�ᣡ",
			__FUNCTION__, m_nFaildCount, m_nHeartCount);
		SetRegister(false);
		Register(0, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	Heart();	
}

//ע��״̬
void CRegister::OnUnRegister()
{
	if (GetTickCount() - m_dwTime < TIMEOUT_REG)
	{
		return;
	}
	if (m_nFaildCount < 3)
	{
		//��ʱ�ˣ��ط�
		Log(Tool::Warning, "[%s]��<%d>��ע�����ʱ���ط�", __FUNCTION__, m_nFaildCount);
		Register(0, "", "", m_strRegFromTag, m_strRegCallID);
		return;
	}
	Log(Tool::Warning, "[%s]ע��ʧ�ܴ���<%d>�ﵽ3�Σ�������ע������ע�ᣡ", __FUNCTION__, m_nFaildCount);
	SetState(eSInit);
}

/*******************************************************************************
*��    ��:	�յ�ע��Ӧ��Ĵ���
*�������:	
*�������: 	
*�� �� ֵ��	
*����˵��:	
*�޸�����	�޸���			�޸�����
--------------------------------------------------------------------------------
2016-8-2	�ű�			����		
*******************************************************************************/
void CRegister::OnRegisterR(CMsgRBase *pCmd, CSubMsg *pSubMsg)
{
	if (eSRegister != m_nState && eSUnregister != m_nState)
	{
		Log(Tool::Warning, "�յ���������ע��Ӧ����Ϣ����ǰ״̬<%d>, ��������", m_nState);
		return;
	}
	m_nFaildCount = 0;
	switch (pCmd->m_FirstLine.nReply)
	{
	case eROK:
		{
			if (eSUnregister == m_nState)
			{
				//ע���ɹ�
				SetState(eSInit);
				SetRegister(false);
				m_strRegFromTag = "";
				m_strRegFromTag = "";
			}
			else if(eSRegister == m_nState)
			{
				//ע��ɹ������̷�������
				SetRegister(true);
				m_strRegCallID = pCmd->GetHead("Call-ID");
				m_strRegFromTag = pCmd->m_From.strTag;
				Heart();
			}
		}		
		break;
	case  eRUnAuthorize: //δ��Ȩ��ע��
		if (!m_bAuth)
		{
			Register(
				eSRegister== m_nState ? m_nExpire/1000 : 0, 
				pCmd->m_Auth.strNonce,
				pCmd->m_Auth.strRealm,
				pCmd->m_From.strTag,
				pCmd->GetHead("Call-ID"),
				pCmd->m_Auth.strOpaque
				);
		}
		else
		{
			Log(Tool::Error, "��Ȩ��Ϣ����ע��/ע��ʧ�ܣ�");
		}
		break;
	default:
		Log(Tool::Warning, "ע��Ӧ��ֵ<%d>, δ����", pCmd->m_FirstLine.nReply);
		break;
	}
}

//�յ�����Ӧ��
void CRegister::OnHeartR(CMsgRBase *pCmd, CSubMsg *pSubMsg)
{
	if(eSHeart != m_nState)
	{
		Log(Tool::Warning, "�յ���������Message��Ϣ����ǰ״̬<%d>,Heart ��������", m_nState);
		return;
	}
	m_nFaildCount = 0;	
	if (Sip::eROK != pCmd->m_FirstLine.nReply)
	{
		Log(Tool::Warning, "����Ӧ����200OK<%d - %s>", pCmd->m_FirstLine.nReply, pCmd->m_FirstLine.strReply.c_str());
		SetState(eSInit);
		SetRegister(false);
		return;
	}
	//Log("�յ�����Ӧ��");
}

//���õ�ǰ״̬
void CRegister::SetState(int nState)
{
	if (nState != m_nState)
	{
		m_nFaildCount = 0;
		m_nState = nState;
	}
	m_dwTime = GetTickCount();
}

//����ע��״̬
void CRegister::SetRegister(bool bRegister)
{
	if (bRegister)
	{
		if (0 == m_dwRegOK)
		{
			Log(Tool::Info, "[%s]<%s>���ߣ� -- ������<%d>", __FUNCTION__, m_strID.c_str(), m_nRegisterCount.fetch_add(1));
			if (NULL != m_pfn)
			{
				m_pfn(m_lpContext, true);
			}
		}
		m_dwRegOK = GetTickCount();
	}
	else
	{
		if (0 != m_dwRegOK)
		{
			Log(Tool::Info, "[%s]<%s>���ߣ� --- ������<%d>", __FUNCTION__, m_strID.c_str(), m_nRegisterCount.fetch_sub(1));
			if (NULL != m_pfn)
			{
				m_pfn(m_lpContext, false);
			}
		}
		m_dwRegOK = 0;
	}
}

//����ע����Ϣ
void CRegister::Register( int nExpire, const std::string &strNonce /* = "" */, const std::string &strRealm /* = "" */, 
						 const std::string &strFromTag /* = "" */, const std::string &strCallID /* = "" */,
						 const std::string &strOpaque /* = "" */ )
{
	m_bAuth = !strRealm.empty();
	CMsg::AuthNode auth;
	auth.strNonce = strNonce;
	auth.strRealm = strRealm;
	auth.strOpaque = strOpaque;
	CMsgReg msg;
	msg.SetParma(m_strServerID, m_strID, m_strLocalIP, m_nLocalPort,
		CCounter::GetInstanse().Get(m_strID+"REGISTER"), auth, m_strPassword, nExpire, strFromTag, strCallID);
	msg.Make();
	//if(!Tool::CUdpY::GetInstance().Send(msg.Str().c_str(), msg.Str().size()))
	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), m_strServerIP, m_nServerPort))
	{
		Log(Tool::Error, "����ע����Ϣʧ�ܣ�");
		return;
	}

	Log(msg.Str().c_str());
	Log("\n");
	SetState(0 != nExpire ? eSRegister : eSUnregister);
	m_strCallID = msg.GetHead("Call-ID");
	m_nFaildCount++;
}

//������Ϣ
void CRegister::Heart()
{
	CSubMsgKeepAlive msgSub;
	msgSub.SetParam(CCounter::GetInstanse().Get(m_strID + "SN_HEART"), m_strID, eStatusOK);
	msgSub.Make();
	CMsgMsg msg;
	msg.SetParam(m_strServerID, m_strID, m_strLocalIP, m_nLocalPort,
		CCounter::GetInstanse().Get(m_strID + "HEART"), msgSub.Str()[0]);
	msg.Make();

	//if(!Tool::CUdpY::GetInstance().Send(msg.Str().c_str(), msg.Str().size()))
	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdp->Send(msg.Str().c_str(), msg.Str().size(), m_strServerIP, m_nServerPort))
	{
		Log(Tool::Error, "����������Ϣʧ�ܣ�");
	}
	m_nFaildCount++;
	m_strCallID = msg.GetHead("Call-ID");
	SetState(eSHeart);
}

}
