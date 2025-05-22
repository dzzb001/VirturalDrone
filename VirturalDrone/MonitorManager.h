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

		//��ȡ���������������
		static CMonitorManager &GetInstance() { return s_MonitorManager; }

		//��ȡ������������
		std::shared_ptr<CMonitor> GetMonitor();
	protected:
		//��̬ʵ������
		static CMonitorManager s_MonitorManager;

		XMutex m_mutexGetMonitor;

		//��ȡ��������������������������ȡ��Ӧ�������������
		int m_nMonitorIndex;
	private:
		std::vector<std::shared_ptr<CMonitor>> m_vecMonitor;
	};
}

