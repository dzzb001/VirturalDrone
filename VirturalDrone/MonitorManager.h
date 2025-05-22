#pragma once
#include "Monitor.h"
#include <vector>
#include <set>
#include "Tool/XMutex.h"

namespace Tool {

	class CMonitorManager
	{
	public:
		CMonitorManager();
		~CMonitorManager();

		void Init();
		void DeInit();

		//获取网络驱动管理对象
		static CMonitorManager &GetInstance() { return s_MonitorManager; }

		//获取单个网络驱动
		std::shared_ptr<CMonitor> GetMonitor();
	protected:
		//静态实例对象
		static CMonitorManager s_MonitorManager;

		XMutex m_mutexGetMonitor;

		//获取网络驱动的索引，根据索引获取对应的网络驱动句柄
		int m_nMonitorIndex;
	private:
		std::vector<std::shared_ptr<CMonitor>> m_vecMonitor;
	};
}

