#include "stdafx.h"
#include "MonitorManager.h"

#define MONITOR_COUNT  (4)

namespace Tool {

	//¾²Ì¬ÊµÀý¶ÔÏó
	CMonitorManager CMonitorManager::s_MonitorManager;

	CMonitorManager::CMonitorManager()
	{
	}

	CMonitorManager::~CMonitorManager()
	{
	}

	void CMonitorManager::Init()
	{
		m_nMonitorIndex = 0;
		for (int i = 0; i < MONITOR_COUNT; i++)
		{
			auto pMonitor = std::make_shared<CMonitor>();
			m_vecMonitor.push_back(pMonitor);
			pMonitor->Init();
		}
	}
	void CMonitorManager::DeInit()
	{
		for (int i = 0; i < MONITOR_COUNT; i++)
		{
			m_vecMonitor[i]->DeInit();
		}
	}

	std::shared_ptr<CMonitor> CMonitorManager::GetMonitor()
	{
		m_mutexGetMonitor.Lock();
		if (m_nMonitorIndex >= MONITOR_COUNT) {
			m_nMonitorIndex = 0;
		}
		std::shared_ptr<CMonitor> pMonitor = m_vecMonitor[m_nMonitorIndex];
		m_nMonitorIndex++;
		m_mutexGetMonitor.UnLock();

		return pMonitor;
	}
}
