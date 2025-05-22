// 2008-02-26
// XThreadPool.cpp
// guoshanhe
// 线程池

#include "XThreadPool.h"

namespace xbase {

///////////////////////////////////////////////////////////////////////////////
// XThreadPool
////////////////////////////////////////////////////////////////////////////////
XThreadPool::XThreadPool()
	: m_nMinThreadCount(1)
	, m_nMaxThreadCount(1)
	, m_nDynamicMinThreadCount(1)
	, m_threadActiveCount(0)
{
	m_mapThreadList.clear();
	m_listDeadThreadList.clear();
	m_listTaskList.clear();
}

XThreadPool::~XThreadPool()
{
	_XTaskThread *pThread = NULL;
	IXThreadTask *pTask = NULL;

	StopAll();

	do
	{
		while (m_listDeadThreadList.size() > 0)
		{
			m_lockThrads.Lock();
			pThread = m_listDeadThreadList.front();
			m_listDeadThreadList.pop_front();
			m_lockThrads.UnLock();
			delete pThread;
		}
	}while(GetThreadTotalCount() > 0);

	m_lockTasks.Lock();
	while (m_listTaskList.size() > 0)
	{
		pTask = m_listTaskList.front();
		m_listTaskList.pop_front();
		delete pTask;
	}
	m_lockTasks.UnLock();
}

// 启动所有子线程
uint32 XThreadPool::StartAll(uint16 nMinThreadCount/* = 1*/, uint16 nMaxThreadCount/* = 0XFFFF*/)
{
	if (GetThreadID() != 0)
	{
		return 0;
	}
	
	m_nMinThreadCount = nMinThreadCount;
	m_nMaxThreadCount = nMaxThreadCount;
	if (m_nMinThreadCount < 1)
	{
		m_nMinThreadCount = 1;
	}
	m_nDynamicMinThreadCount = m_nMinThreadCount;

	if (m_nMaxThreadCount < m_nMinThreadCount)
	{
		m_nMaxThreadCount = m_nMinThreadCount;
	}
	if (m_nMaxThreadCount > 0XFFFF)
	{
		m_nMaxThreadCount = 0XFFFF;
	}
	
	// 启动线程池管理线程
	if (!XThread::Start()) return 0;

	_XTaskThread *pThread = NULL;
	uint32 nTotal = 0;
	for (uint32 i = 0; i < m_nMinThreadCount; i++)
	{
		pThread = new(std::nothrow) _XTaskThread(this);
		if (pThread == NULL) continue;

		if (!pThread->Start())
		{
			delete pThread;
			pThread = NULL;
			continue;
		}

		m_lockThrads.Lock();
		m_mapThreadList[pThread->GetThreadID()] = pThread;
		m_lockThrads.UnLock();
	}

	m_lockThrads.Lock();
	nTotal = (uint32)m_mapThreadList.size();
	m_lockThrads.UnLock();
	
	return nTotal;
}

// 停止所有子线程
void XThreadPool::StopAll(void)
{
	THREAD_MAP_TYPE::iterator it;
	_XTaskThread *pThread = NULL;

	// 停止线程池管理线程
	XThread::PostStop();
	Join();

	// 发送停止信号
	m_lockThrads.Lock();
	it = m_mapThreadList.begin();
	while (it != m_mapThreadList.end())
	{
		pThread = it->second;
		if (pThread) pThread->PostStop();
		++it;
	}
	m_lockThrads.UnLock();

	return;
}

// 获取当前可用线程总数(睡眠和活跃)
uint32 XThreadPool::GetThreadTotalCount(void)
{
	XLockGuard<XCritical> lock(m_lockThrads);
	return (uint32)m_mapThreadList.size();
}

// 获取活跃线程数
uint32 XThreadPool::GetThreadActiveCount(void)
{
	return (uint32)m_threadActiveCount;
}

// 指派一个任务
void XThreadPool::AddTask(IXThreadTask *pTask)
{
	if (!pTask) return;

	m_lockTasks.Lock();
	m_listTaskList.push_back(pTask);
	m_lockTasks.UnLock();

	// 发出通知
	m_evWork.Set();

	return;
}

// 获取当前队列中任务数
uint32 XThreadPool::GetTaskCount(void)
{
	XLockGuard<XCritical> lock(m_lockTasks);
	return (uint32)m_listTaskList.size();
}

// 线程池管理线程入口
void XThreadPool::Entry()
{
	_XTaskThread *pThread = NULL;

	while (!TryWaitQuit(10))
	{
		// 计算增加线程数
		uint32 nAddCount = NeedStartThreadCount();
		if (nAddCount > 0)
		{
			m_nDynamicMinThreadCount = GetThreadTotalCount() + nAddCount;
			if (m_nDynamicMinThreadCount > m_nMaxThreadCount)
			{
				m_nDynamicMinThreadCount = m_nMaxThreadCount;
				nAddCount = m_nMaxThreadCount - GetThreadTotalCount();
			}
		}

		for (uint32 i = 0; i < nAddCount && !TryWaitQuit(); i++)
		{
			pThread = new(std::nothrow) _XTaskThread(this);
			if (pThread)
			{
				if (!pThread->Start())
				{
					delete pThread;
					pThread = NULL;
					continue;
				}
				m_lockThrads.Lock();
				m_mapThreadList[pThread->GetThreadID()] = pThread;
				m_lockThrads.UnLock();
			}
		}

		// 等待死亡队列子线程结束
		while (!TryWaitQuit())
		{
			pThread = NULL;
			m_lockThrads.Lock();
			if ((uint32)m_listDeadThreadList.size() > 0)
			{
				pThread = m_listDeadThreadList.front();
				m_listDeadThreadList.pop_front();
			}
			m_lockThrads.UnLock();
			if (pThread)
			{
				delete pThread;
				pThread = NULL;
				continue;
			}

			break;
		}

		// 每5秒采样一次
		uint32 count = 0;
		if (++count % 500 == 0)
		{
			count = 0;

			if (m_nDynamicMinThreadCount > GetThreadTotalCount() * 90 / 100)
			{
				// 调整动态最小线程数为90%，为了使线程的创建和销毁趋于缓和
				m_nDynamicMinThreadCount = m_nDynamicMinThreadCount * 90 / 100;
				if (m_nDynamicMinThreadCount < m_nMinThreadCount)
				{
					m_nDynamicMinThreadCount = m_nMinThreadCount;
				}
			}
		}
	}

	return;
}

void XThreadPool::_MoveThreadToDeadList(_XTaskThread *pThread)
{
	if (!pThread) return;

	m_lockThrads.Lock();
	m_mapThreadList.erase(pThread->GetThreadID());
	m_listDeadThreadList.push_back(pThread);
	m_lockThrads.UnLock();

	return;
}

void XThreadPool::AttachThreadToTask(XThread *pThread, IXThreadTask *pTask)
{
	if (pTask == NULL) return;
	pTask->m_pThread = pThread;
	return;
}

// 判断当前需要增加的线程数
uint32 XThreadPool::NeedStartThreadCount()
{
	uint32 nAddCount = GetTaskCount();
	uint32 nTotalCount = GetThreadTotalCount();
	uint32 nActiveCount = GetThreadActiveCount();
	uint32 nSleepCount = nTotalCount - nActiveCount;
	if (nAddCount > nSleepCount)
	{
		nAddCount -= nSleepCount;
		if (nAddCount + nTotalCount > m_nMaxThreadCount)
		{
			nAddCount = m_nMaxThreadCount - nTotalCount;
		}
	}
	else
	{
		nAddCount = 0;
	}

	return nAddCount;
}

// 判断当前是否需要退出线程
BOOL XThreadPool::IsNeedStopThread()
{
	// 判断是否退出线程
	uint32 nActiveCount = GetThreadActiveCount();
	uint32 nTotalCount = GetThreadTotalCount();
	uint32 nSleepCount = nTotalCount - nActiveCount;
	if (nTotalCount <= 1) return FALSE;
	if ((nSleepCount * 100 / nTotalCount > 10) && 
		(nTotalCount > m_nDynamicMinThreadCount))
	{
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// XThreadPool::_XTaskThread
////////////////////////////////////////////////////////////////////////////////
void XThreadPool::_XTaskThread::Entry()
{
	while (!TryWaitQuit())
	{
		if (!m_pThreadPool->m_evWork.TryWait(250))
		{
			if (m_pThreadPool->IsNeedStopThread())
				break;
			else
				continue;
		}

		// 等到信号,活跃线程数增加
		++m_pThreadPool->m_threadActiveCount;

		// 循环取任务执行
		while (!TryWaitQuit())
		{
			IXThreadTask *pTask = NULL;
			m_pThreadPool->m_lockTasks.Lock();
			if ((uint32)m_pThreadPool->m_listTaskList.size() > 0)
			{
				pTask = m_pThreadPool->m_listTaskList.front(); 
				m_pThreadPool->m_listTaskList.pop_front();
			}
			m_pThreadPool->m_lockTasks.UnLock();
			if (pTask)
			{
				m_pThreadPool->AttachThreadToTask(this, pTask);
				pTask->Run();
				delete pTask;
				continue;
			}
			
			break;
		}

		// 任务执行完毕,线程即将进入睡眠,活跃线程数减少
		--m_pThreadPool->m_threadActiveCount;
		if (m_pThreadPool->m_threadActiveCount < 0)
			m_pThreadPool->m_threadActiveCount = 0;
	}

	m_pThreadPool->_MoveThreadToDeadList(this);
	return;
}

} // namespace xbase
