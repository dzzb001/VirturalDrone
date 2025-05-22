// 2008-02-26
// XThreadPool.cpp
// guoshanhe
// �̳߳�

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

// �����������߳�
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
	
	// �����̳߳ع����߳�
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

// ֹͣ�������߳�
void XThreadPool::StopAll(void)
{
	THREAD_MAP_TYPE::iterator it;
	_XTaskThread *pThread = NULL;

	// ֹͣ�̳߳ع����߳�
	XThread::PostStop();
	Join();

	// ����ֹͣ�ź�
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

// ��ȡ��ǰ�����߳�����(˯�ߺͻ�Ծ)
uint32 XThreadPool::GetThreadTotalCount(void)
{
	XLockGuard<XCritical> lock(m_lockThrads);
	return (uint32)m_mapThreadList.size();
}

// ��ȡ��Ծ�߳���
uint32 XThreadPool::GetThreadActiveCount(void)
{
	return (uint32)m_threadActiveCount;
}

// ָ��һ������
void XThreadPool::AddTask(IXThreadTask *pTask)
{
	if (!pTask) return;

	m_lockTasks.Lock();
	m_listTaskList.push_back(pTask);
	m_lockTasks.UnLock();

	// ����֪ͨ
	m_evWork.Set();

	return;
}

// ��ȡ��ǰ������������
uint32 XThreadPool::GetTaskCount(void)
{
	XLockGuard<XCritical> lock(m_lockTasks);
	return (uint32)m_listTaskList.size();
}

// �̳߳ع����߳����
void XThreadPool::Entry()
{
	_XTaskThread *pThread = NULL;

	while (!TryWaitQuit(10))
	{
		// ���������߳���
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

		// �ȴ������������߳̽���
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

		// ÿ5�����һ��
		uint32 count = 0;
		if (++count % 500 == 0)
		{
			count = 0;

			if (m_nDynamicMinThreadCount > GetThreadTotalCount() * 90 / 100)
			{
				// ������̬��С�߳���Ϊ90%��Ϊ��ʹ�̵߳Ĵ������������ڻ���
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

// �жϵ�ǰ��Ҫ���ӵ��߳���
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

// �жϵ�ǰ�Ƿ���Ҫ�˳��߳�
BOOL XThreadPool::IsNeedStopThread()
{
	// �ж��Ƿ��˳��߳�
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

		// �ȵ��ź�,��Ծ�߳�������
		++m_pThreadPool->m_threadActiveCount;

		// ѭ��ȡ����ִ��
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

		// ����ִ�����,�̼߳�������˯��,��Ծ�߳�������
		--m_pThreadPool->m_threadActiveCount;
		if (m_pThreadPool->m_threadActiveCount < 0)
			m_pThreadPool->m_threadActiveCount = 0;
	}

	m_pThreadPool->_MoveThreadToDeadList(this);
	return;
}

} // namespace xbase
