// 2008-02-26
// XThreadPool.h
// guoshanhe
// 线程池


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

// 线程池执行任务体接口
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

	// 检测是否收到退出信号
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


// 线程池类
///////////////////////////////////////////////////////////////////////////////
// XThreadPool
////////////////////////////////////////////////////////////////////////////////
class XThreadPool : public XThread
{
private:
	// 线程池执行任务线程类
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
	
	friend class _XTaskThread;	// 设为友员,以便检测信号灯(semaphore)

private:
	typedef map<uint32, _XTaskThread *> THREAD_MAP_TYPE;
	typedef list<_XTaskThread *> THREAD_LIST_TYPE;
	typedef list<IXThreadTask *> THREAD_TASK_LIST_TYPE;

public:
	XThreadPool();

	virtual ~XThreadPool();

	// 启动所有线程
	uint32 StartAll(uint16 nMinThreadCount = 1, uint16 nMaxThreadCount = 0XFFFF);

	// 停止所有线程
	void StopAll(void);

	// 获取当前可用线程总数(睡眠和活跃)
	uint32 GetThreadTotalCount(void);

	// 获取当前活跃线程数
	uint32 GetThreadActiveCount(void);

	// 获取当前队列中任务数
	uint32 GetTaskCount(void);

	// 添加任务
	void AddTask(IXThreadTask *pTask);

protected:
	// 判断当前需要增加的线程数
	virtual uint32 NeedStartThreadCount();

	// 判断当前是否需要退出一个子线程
	virtual BOOL IsNeedStopThread();

private:
	// 线程池管理线程入口
	virtual void Entry();

	void _MoveThreadToDeadList(_XTaskThread *pThread);

	void AttachThreadToTask(XThread *pThread, IXThreadTask *pTask);

	// 不要使用下面两种启停方式
	virtual BOOL Start(PFTHREADPROC pfThreadProc = NULL, void *pParam = NULL) { return FALSE; };
	virtual void PostStop() {};

private:
	uint32					m_nMinThreadCount;			// 允许的最小线程数
	uint32					m_nMaxThreadCount;			// 允许的最大线程数
	uint32					m_nDynamicMinThreadCount;	// 动态变化的最小线程数

	XAtomic					m_threadActiveCount;		// 当前活跃线程总数

	THREAD_MAP_TYPE			m_mapThreadList;			// 当前可用线程列表
	THREAD_LIST_TYPE		m_listDeadThreadList;		// 已死亡等待回收线程列表
	XCritical				m_lockThrads;				// 锁

	THREAD_TASK_LIST_TYPE	m_listTaskList;				// 当前未处理任务列表
	XCritical				m_lockTasks;				// 锁
	XEvent					m_evWork;					// 任务事件
};

} // namespace xbase

using namespace xbase;

#endif//_X_THREAD_POOL_H_

