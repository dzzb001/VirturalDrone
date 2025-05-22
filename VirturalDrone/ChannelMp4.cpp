#include "stdafx.h"
#include "ChannelMp4.h"

#define Log LogN(4002)

CChannelMp4::CChannelMp4(const std::string &strDevID, const std::string &strID,
	const std::string &strName, const std::string &strRtspUrl,
	const std::string &strPayload)
	: CChannel(strDevID, strID, strName),m_strRtspUrl(strRtspUrl)
{
	m_pFileStream = std::make_shared<CMeadiaPaser>();
	m_strPayload = "H264";

	//启动mqtt数据发送
	m_pFileStream->InitMqtt(strID);
}

CChannelMp4::~CChannelMp4()
{
	if (m_pFileStream) {
		m_pFileStream->Stop();
		m_pFileStream->DeInitMqtt();
	}
}

//

//命令输入
void CChannelMp4::CmdIn(Sip::CMsg* pCmd, Sip::CSubMsg *pSubCmd)
{
	if (pCmd->m_To.strID != m_strID)
	{
		Log(Tool::Warning, "[%s] Invite 命令的ToID<%s> != 本通道ID<%s>，丢弃！", __FUNCTION__,
			pCmd->m_To.strID.c_str(), m_strID.c_str());
		return;
	}
	else {
		Log("[%s] Invite 命令的ToID<%s>, 本通道ID<%s>，丢弃！", __FUNCTION__,
			pCmd->m_To.strID.c_str(), m_strID.c_str());
	}

	//趁机先检查下有没有僵死的会话
	std::vector<std::string> vecDeadCallID;
	for (ClientMap::iterator it = m_mapClient.begin(); it != m_mapClient.end(); ++it)
	{
		if (it->second->Dead())
		{
			vecDeadCallID.push_back(it->first);
		}
	}
	for (size_t i = 0; i < vecDeadCallID.size(); ++i)
	{
		Log(Tool::Info, "[%s]会话<%s>已死，将被删除！", __FUNCTION__, vecDeadCallID[i].c_str());
		Delete(vecDeadCallID[i]);
	}

	//立马返回tring
	if (Sip::eCmdInvite == pCmd->m_nType)
		Reply(Sip::eRTrying, pCmd, pSubCmd);
	
	//再处理命令
	if (Sip::eCmdInvite == pCmd->m_nType && m_pFileStream && m_pFileStream->Start(m_strRtspUrl.c_str()))
	{
		Reply(Sip::eRNotFound, pCmd, pSubCmd);
		if (m_pFileStream) {
			m_pFileStream->Stop();
		}
		return;
	}
	if (Sip::eCmdInvite == pCmd->m_nType && m_pFileStream)
	{
		m_pFileStream->StartMqtt();
	}
	ExeCmd(pCmd, pSubCmd);

	////没有客户端观看了就把视频源关掉
	if (m_mapClient.empty())
	{
		Log(Tool::Info, "[%s]本通道没有客户端了，关闭数据源！", m_strRtspUrl.c_str());
		if (m_pFileStream) {
			m_pFileStream->Stop();
			m_pFileStream->EndMqtt();
		}
	}
}

void CChannelMp4::ClientIn(std::weak_ptr<CTcpClientBase> pClient, const std::string &strCallID)
{
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
}

//关闭通道，已关闭成功返回true，否则返回false
bool CChannelMp4::Close()
{
	//Log("[%s] ID<%s>", __FUNCTION__, m_strID.c_str());
	for (size_t i = 0; i < m_vecDst.size(); ++i)
	{
		m_vecDst[i]->Close();
		m_vecDst[i].reset();
	}
	m_vecDst.clear();
	for (ClientMap::iterator it = m_mapClient.begin(); it != m_mapClient.end(); ++it)
	{
		it->second->Close();
		it->second.reset();
	}
	if (m_pFileStream) {
		m_pFileStream->Stop();
		m_pFileStream->EndMqtt();
	}
	m_mapClient.clear();
	m_mapSubject.clear();

	return true;
}

/*******************************************************************************
*功    能:	执行异步Sip命令
*输入参数:	mapClient		-- CallID -> 发送客户端 列表
*			mapSubject		-- Subject -> CallID 列表
*输出参数:
*返 回 值：	无
*其它说明:
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-11	张斌			创建
*******************************************************************************/
void CChannelMp4::ExeCmd(Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd)
{
	std::string strCallID = pCmd->GetHead("Call-ID");
	ClientMap::iterator it = m_mapClient.find(strCallID);
	if (m_mapClient.end() == it)
	{
		if (Sip::eCmdInvite != pCmd->m_nType)
		{
			Log(Tool::Warning, "[%s]找不到命令所属的CallID信息！", __FUNCTION__);
			if (Sip::eCmdBye == pCmd->m_nType)
			{
				Reply(Sip::eRForbidden, pCmd, pSubCmd);
			}
			return;
		}

		//检查有没有需要停掉的重复客户端
		std::string strSubject = pCmd->GetHead("Subject");
		std::unordered_map<std::string, std::string> ::iterator itSubject = m_mapSubject.find(strSubject);
		if (m_mapSubject.end() != itSubject)
		{
			Delete(itSubject->second);
		}

		//新建一个客户端
		m_mapSubject[strSubject] = strCallID;
		
		auto pClient = CContextBase::Create<CClient>();
		pClient->SetParam(m_strDevID, m_strPayload, m_strRtspUrl.c_str());
		pClient->RegCB(RtpNotifyCB, AddrCB, this);
		m_mapClient[strCallID] = pClient;
		it = m_mapClient.find(strCallID);
	}
	it->second->CmdIn(pCmd, pSubCmd);
	if (it->second->Dead())
	{
		Delete(strCallID);
	}
}

/*******************************************************************************
*功    能:	删除一个客户端
*输入参数:	strCallID		-- 要删除客户端的ID
*输出参数:
*返 回 值：
*其它说明:	输入参数strCallID，不要使用引用，以防止被删除后再引用出错。
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-8-15	张斌			创建
*******************************************************************************/
void CChannelMp4::Delete(std::string strCallID)
{
	ClientMap::iterator it = m_mapClient.find(strCallID);
	if (m_mapClient.end() != it)
	{
		m_lockDst.Lock();
		std::vector<std::shared_ptr<CClient>>::iterator itDst = std::find(m_vecDst.begin(), m_vecDst.end(), it->second);
		if (m_vecDst.end() != itDst)
		{
			m_vecDst.erase(itDst);
		}
		m_lockDst.UnLock();

		it->second->Close();
		it->second.reset();
		//delete it->second;
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
}

void CChannelMp4::RtpNotifyCB(LPVOID lpContext, void *pClient, bool bStart)
{
	CChannelMp4 *pThis = (CChannelMp4*)lpContext;
	CClient* pClient1 = (CClient*)pClient;

	if (pThis) {

		if (bStart)
		{
			Log(Tool::Debug, "RtpNotifyCB bStart=true\n");
			/*pThis->m_rtsp->AddClient(pClient1->m_strCallID, CClient::RtpDataCB, pClient1);*/
			pThis->m_pFileStream->AddClient(pClient1->m_strCallID, CClient::RtpDataCB, pClient1);
		}
		else {
			Log(Tool::Debug, "RtpNotifyCB bStart=false\n");
			/*pThis->m_rtsp->RemoveClient(pClient1->m_strCallID);*/
			pThis->m_pFileStream->RemoveClient(pClient1->m_strCallID);
		}
	}
}


//应答命令
void CChannelMp4::Reply(int nReply, Sip::CMsg *pCmd, Sip::CSubMsg *pSubCmd, const std::string &strContent/* ="" */)
{
	Sip::CMsgRBase msg;
	msg.Reply(nReply, (Sip::CMsgBase*)pCmd, strContent);
	msg.Make();
	std::shared_ptr<CUdpM> pUdpM = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	if (!pUdpM->Send(msg.Str().c_str(), msg.Str().size(), Tool::CUdpM_G::GetInstance().GetServerIP(), Tool::CUdpM_G::GetInstance().GetServerPort()))
	{
		Log(Tool::Error, "[%s]发送应答失败！", __FUNCTION__);
	}
}

/*******************************************************************************
*功    能:	解析文件类型字符串，形如：PS/pcap
*输入参数:	str				-- 待解析的字符串
*输出参数: 	vec				-- 解析出的子字符串
*返 回 值：
*其它说明:	根据"/"分隔
*修改日期	修改人			修改内容
--------------------------------------------------------------------------------
2016-10-19	张斌			创建
*******************************************************************************/
void CChannelMp4::SpliteStr(const std::string str, std::vector<std::string> &vec)
{
	std::string::size_type nStart = str.find_first_not_of('/');
	while (std::string::npos != nStart)
	{
		std::string::size_type nEnd = str.find_first_of('/', nStart);
		if (std::string::npos == nEnd)
		{
			vec.push_back(str.substr(nStart, str.size() - nStart));
			return;
		}
		vec.push_back(str.substr(nStart, nEnd - nStart));
		nStart = str.find_first_not_of('/', nEnd);
	}
}

void CChannelMp4::AddrCB(LPVOID lpContext, const std::string &strCallID, const std::string &strRemoteIP, int nRemotePort)
{
	CChannelMp4 *pThis = (CChannelMp4*)lpContext;

#if 0	
	if (nullptr != pThis->m_pfnAddr)
	{
		pThis->m_pfnAddr(pThis->m_lpContext, pThis->m_strID, strCallID, strRemoteIP, nRemotePort);
	}
#endif
}

void CChannelMp4::CheckClient()
{
	//检查下有没有僵死的会话
	std::vector<std::string> vecDeadCallID;
	for (ClientMap::iterator it = m_mapClient.begin(); it != m_mapClient.end(); ++it)
	{
		if (it->second->Dead())
		{
			vecDeadCallID.push_back(it->first);
		}
	}

	for (size_t i = 0; i < vecDeadCallID.size(); ++i)
	{
		Log(Tool::Info, "[%s]会话<%s>已死，将被删除！", __FUNCTION__, vecDeadCallID[i].c_str());
		Delete(vecDeadCallID[i]);
	}
	//Log(Tool::Info, "CheckClient %s", m_strRtspUrl.c_str());
	//没有客户端观看了就把视频源关掉
	if (m_mapClient.empty())
	{
		if (m_pFileStream) {
			m_pFileStream->Stop();
			m_pFileStream->EndMqtt();
		}
	}
}