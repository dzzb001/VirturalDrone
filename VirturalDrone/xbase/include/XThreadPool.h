// 2008-02-26
// XThreadPool.h
// guoshanhe
// �̳߳�


#pragma once

#ifndef _X_THREAD_POOL_H_
#define _X_THREAD_POOL_H_

#include "XDefine.h"
#include "XThread.h"
#include "XCritical.h"
#include "XSemaphore.h"
#include "XAtomic.h"
#include "XEvent.h"

namespace xbase {

// �̳߳�ִ��������ӿ�
///////////////////////////////////////////////////////////////////////////////
// IXThreadTask
////////////////////////////////////////////////////////////////////////////////
class IXThreadTask
{
	friend class XThreadPool;
public:
	IXThreadTask()
		: m_pThread(NULL)
	{
		// empty
	}

	virtual ~IXThreadTask()
	{
		// empty
	}

	// ����Ƿ��յ��˳��ź�
	virtual BOOL TryWaitQuit(uint32 uMilliseconds = 0)
	{
		if (m_pThread)
		{
			return m_pThread->TryWaitQuit(uMilliseconds);
		}
		return TRUE;
	}

	virtual void Run() = 0;

private:
	XThread *	m_pThread;
};


// �̳߳���
///////////////////////////////////////////////////////////////////////////////
// XThreadPool
////////////////////////////////////////////////////////////////////////////////
class XThreadPool : public XThread
{
private:
	// �̳߳�ִ�������߳���
	class _XTaskThread : public XThread
	{
	public:
		_XTaskThread(XThreadPool *pThreadPool)
			: m_pThreadPool(pThreadPool)
		{
			// empty
		}

		virtual ~_XTaskThread()
		{
			PostStop();
			Join();
		}

	private:
		void Entry();

	private:
		XThreadPool *m_pThreadPool;
	};
	
	friend class _XTaskThread;	// ��Ϊ��Ա,�Ա����źŵ�(semaphore)

private:
	typedef map<uint32, _XTaskThread *> THREAD_MAP_TYPE;
	typedef list<_XTaskThread *> THREAD_LIST_TYPE;
	typedef list<IXThreadTask *> THREAD_TASK_LIST_TYPE;

public:
	XThreadPool();

	virtual ~XThreadPool();

	// ���������߳�
	uint32 StartAll(uint16 nMinThreadCount = 1, uint16 nMaxThreadCount = 0XFFFF);

	// ֹͣ�����߳�
	void StopAll(void);

	// ��ȡ��ǰ�����߳�����(˯�ߺͻ�Ծ)
	uint32 GetThreadTotalCount(void);

	// ��ȡ��ǰ��Ծ�߳���
	uint32 GetThreadActiveCount(void);

	// ��ȡ��ǰ������������
	uint32 GetTaskCount(void);

	// �������
	void AddTask(IXThreadTask *pTask);

protected:
	// �жϵ�ǰ��Ҫ���ӵ��߳���
	virtual uint32 NeedStartThreadCount();

	// �жϵ�ǰ�Ƿ���Ҫ�˳�һ�����߳�
	virtual BOOL IsNeedStopThread();

private:
	// �̳߳ع����߳����
	virtual void Entry();

	void _MoveThreadToDeadList(_XTaskThread *pThread);

	void AttachThreadToTask(XThread *pThread, IXThreadTask *pTask);

	// ��Ҫʹ������������ͣ��ʽ
	virtual BOOL Start(PFTHREADPROC pfThreadProc = NULL, void *pParam = NULL) { return FALSE; };
	virtual void PostStop() {};

private:
	uint32					m_nMinThreadCount;			// �������С�߳���
	uint32					m_nMaxThreadCount;			// ���������߳���
	uint32					m_nDynamicMinThreadCount;	// ��̬�仯����С�߳���

	XAtomic					m_threadActiveCount;		// ��ǰ��Ծ�߳�����

	THREAD_MAP_TYPE			m_mapThreadList;			// ��ǰ�����߳��б�
	THREAD_LIST_TYPE		m_listDeadThreadList;		// �������ȴ������߳��б�
	XCritical				m_lockThrads;				// ��

	THREAD_TASK_LIST_TYPE	m_listTaskList;				// ��ǰδ���������б�
	XCritical				m_lockTasks;				// ��
	XEvent					m_evWork;					// �����¼�
};

} // namespace xbase

using namespace xbase;

#endif//_X_THREAD_POOL_H_

