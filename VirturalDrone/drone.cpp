#include "stdafx.h"
#include "drone.h"
#include "Config.h"
#include "UdpM_G.h"
#include "MonitorManager.h"
#define Log LogN(5000)

#define SERVER_NAME "Virtual drone"
#define VERSION_NUM "1.0.0.1"
#define CHECK_TIME  10

CDrone::CDrone()
{
}

CDrone::~CDrone()
{
}

bool CDrone::Init()
{
	print_version();
	//加载配置
	Config::CConfig &config = Config::CConfig::GetInstance();
	if (!config.Load())
	{
		Log(Tool::Error, "load config failed!");
		return false;
	}

	//日志初始化
	int nKeepDays = config.GetKeepDays();
	int nLevel = config.GetLevel();
	Tool::TLog::Init(("log"), ("drone"), nKeepDays); 
	Tool::TLog::SetWriteFile(true);
	Tool::TLog::SetLogLevel(nLevel);
	
	const Config::DevMap &mapDev = config.GetDev();
	for (Config::DevMap::const_iterator it = mapDev.begin(); it != mapDev.end(); ++it)
	{
		const Config::DevNode &node = it->second;
		CDevice *pDevice = new CDevice;
		//pDevice->RegCB(AddrCB, this);
		pDevice->SetParam(config.GetLocalIP(), config.GetLocalPort(),
			config.GetServerIP(), config.GetServerPort(), config.GetServerID(), it->first, node.strPassword, node.nExpire,
			node.nHeartInterval, node.nHeartCount);
		m_vecDevice.push_back(pDevice);
		m_mapID2Device[it->first] = pDevice;
		for (Config::ChMap::const_iterator itCh = node.mapCh.begin(); itCh != node.mapCh.end(); ++itCh)
		{
			pDevice->AddCh(
				itCh->second.strType,
				itCh->first,
				itCh->second.strName,
				itCh->second.strFile,
				itCh->second.strRecord,
				itCh->second.strPayLoad,
				config.GetLocalIP(),
				config.GetLocalPort(),
				config.GetServerIP(),
				config.GetServerPort(),
				config.GetServerID()
				);
			DeviceMap::iterator itID2Dev = m_mapID2Device.find(itCh->first);
			if (m_mapID2Device.end() == itID2Dev)
			{
				m_mapID2Device[itCh->first] = pDevice;
				continue;
			}
			if (itID2Dev->second != pDevice)
			{
				Log(Tool::Error, "config err：ch ID[%s]，Repetition will be ignored！", itCh->first);
			}
		}
	}
	
	Tool::CMonitorManager::GetInstance().Init();
	m_pMonitor = Tool::CMonitorManager::GetInstance().GetMonitor();
	
	Tool::CUdpM_G::GetInstance().Init();
	Tool::CUdpM_G::GetInstance().SetParam(config.GetLocalIP(),config.GetLocalPort(), config.GetServerIP(), config.GetServerPort() );

	int nLocalPort = config.GetLocalPort();
	std::shared_ptr<CUdpM> pUdp = Tool::CUdpM_G::GetInstance().GetUdpInstance();

	pUdp->RegDataRecv(CDrone::UdpDataCB, /*m_pThis.lock()*/(std::shared_ptr<void>)this);
	pUdp->SetMonitor(m_pMonitor);

	if (!pUdp->Start(0, 0, false,nLocalPort, config.GetLocalIP().c_str()))
	{
		Log(Tool::Error, "net start rev failed,please check config!");
		return false;
	}

	m_bStop = false;
	m_bThreadQuit = false;
	Start(); //启动线程
	printf("Init ok!\n");

	return true;
}

void CDrone::print_version()
{
	FILE *fd = fopen("Version.txt", "w+");
	if (fd)
	{
		fprintf(fd, "%s Version:%s, build at %s %s", SERVER_NAME, VERSION_NUM, __X_DATE__, __X_TIME__);
		fclose(fd);
	}
}

bool CDrone::Deinit()
{
#if 1
	m_bStop = true;

	//等待线程退出
	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (!m_bThreadQuit);

	std::shared_ptr<CUdpM> pUdpM = Tool::CUdpM_G::GetInstance().GetUdpInstance();
	pUdpM->QStop();
	{
		std::weak_ptr<CUdpM> p = pUdpM;
		pUdpM.reset();
		CContextBase::WaitDestroy(p);
	}

	Tool::CMonitorManager::GetInstance().DeInit();

	//清理资源
	for (size_t i = 0; i < m_vecDevice.size(); ++i)
	{
		delete m_vecDevice[i];
	}
	m_vecDevice.clear();
	for (size_t i = 0; i < m_vecSipCmd.size(); ++i)
	{
		delete m_vecSipCmd[i].first;
		delete m_vecSipCmd[i].second;
	}
	m_vecSipCmd.clear();
	m_mapID2Device.clear();
#endif
	return true;
}

/*******************************************************************************
*功    能:	Udp数据接收处理回调函数
*输入参数:	lpContext	-- 环境变量
*			pData		-- 接收到的数据指针
*			nLen		-- 接收到的数据长度
*			strIP		-- 数据源IP
*			nPort		-- 数据源端口
*输出参数:
*返 回 值：	无
*其它说明:
*修改日期	修改人		修改内容
--------------------------------------------------------------------------------
2016-7-27	张斌		创建
*******************************************************************************/

void CDrone::UdpDataCB(std::shared_ptr<void> pContext, const char *pData, int nLen, const std::string &strFromIP, short nFromPort)
{
	//Log("接收到来自<%s:%d>的消息", strFromIP.c_str(), nFromPort);
	//Log(std::string((char*)pData, nLen).substr(0, 1000).c_str());
	CDrone *pThis = (CDrone*)(pContext.get());
	Config::CConfig &config = Config::CConfig::GetInstance();
	/*if (0 != config.GetServerIP().compare((LPCSTR)strIP) || config.GetServerPort() != nPort)
	{
	Log("不接受来自<%s:%d>的控制命令，丢弃", strIP, nPort);
	return;
	}*/
	std::pair<Sip::CMsg*, Sip::CSubMsg*> pairMsg = Sip::CMsgParser::Parser((char*)pData, nLen);
	if (NULL == pairMsg.first)
	{
		Log(Tool::Error, "解析命令失败，丢弃！");
		return;
	}
	pairMsg.first->SetSrcPort(nFromPort);

#if 0
	if ((Sip::eCmdMessage == pairMsg.first->m_nType || Sip::eCmdInvite == pairMsg.first->m_nType
		|| Sip::eCmdAck == pairMsg.first->m_nType || Sip::eCmdBye == pairMsg.first->m_nType)
		&& pairMsg.first->m_From.strID != config.GetServerID())
	{
		Log(Tool::Warning, "命令的FromID<%s> != 服务器ID<%s>, 丢弃++++！", pairMsg.first->m_From.strID.c_str(), config.GetServerID().c_str());
		//return;
	}
#endif

	Log(Tool::Warning, "命令的FromID<%s> != 服务器ID<%s>, 丢弃++++！", pairMsg.first->m_From.strID.c_str(), config.GetServerID().c_str());

	pThis->m_LockSipCmd.Lock();
	pThis->m_vecSipCmd.push_back(pairMsg);
	pThis->m_LockSipCmd.UnLock();
}

void CDrone::Entry()
{
	Log(Tool::Debug, "[%s]work thread start！", __FUNCTION__);
	while (!m_bStop)
	{
		ProSipCmd();
		for (size_t i = 0; i < m_vecDevice.size(); ++i)
		{
			m_vecDevice[i]->Process();
		}
		
		//检测rtcp超时
		static long long lCurTime=time(NULL);
		if (time(NULL) - lCurTime >= CHECK_TIME) {
			CheckChannelClient();
			lCurTime = time(NULL);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	Log(Tool::Debug, "工作线程准备退出！");
	bool bUnregisterd = true;
	do
	{
		ProSipCmd();
		bUnregisterd = true;
		for (size_t i = 0; i < m_vecDevice.size(); ++i)
		{
			if (!m_vecDevice[i]->Unregister())
			{
				bUnregisterd = false;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	} while (!bUnregisterd);

	m_bThreadQuit = true;
	Log(Tool::Debug, "[%s]work thread quit！", __FUNCTION__);
}

void CDrone::ProSipCmd()
{
	std::vector<std::pair<Sip::CMsg*, Sip::CSubMsg*> > vecSipCmd;
	m_LockSipCmd.Lock();
	vecSipCmd.swap(m_vecSipCmd);
	m_LockSipCmd.UnLock();
	for (size_t i = 0; i < vecSipCmd.size(); ++i)
	{
		std::pair<Sip::CMsg*, Sip::CSubMsg*> pairMsg = vecSipCmd[i];
		LogN(557)("开始处理一条命令<%d %s>", pairMsg.first->m_CSeq.nNo, pairMsg.first->m_CSeq.strCmd.c_str());
		DeviceMap::iterator it = m_mapID2Device.find(pairMsg.first->m_To.strID);
		if (m_mapID2Device.end() == it)
		{
			it = m_mapID2Device.find(pairMsg.first->m_From.strID);
		}
		if (m_mapID2Device.end() != it)
		{
			it->second->CmdIn(pairMsg.first, pairMsg.second);
		}
		else
		{
			Log(Tool::Warning, "[%s]找不到命令对应的设备，丢弃！", __FUNCTION__);
		}
		delete pairMsg.first;
		delete pairMsg.second;
	}
}

//检测客户端连接
void CDrone::CheckChannelClient()
{
	DeviceMap::iterator itor = m_mapID2Device.begin();
	for (; itor != m_mapID2Device.end(); itor++)
	{
		itor->second->CheckClient();
	}
}

