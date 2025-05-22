#include "stdafx.h"
#include "ThreadP.h"
  
namespace Tool
{

//预置的三个线程池
CThreadP TP1(THREAD_PRIORITY_NORMAL);
CThreadP TP2(THREAD_PRIORITY_HIGHEST);
CThreadP TP3(THREAD_PRIORITY_TIME_CRITICAL);

/*******************************************************************************
* 功能描述：	构造函数。
* 输入参数：	nPriority	-- 线程优先级，其参数与AfxBeginThread相同。
* 输出参数：	
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2012-12-07	周锋	      	创建
*******************************************************************************/
CThreadP::CThreadP(int nPriority) : m_nPriority(nPriority)
{
	m_bStop = false;
}

/*******************************************************************************
* 功能描述：	析构函数。
* 其它说明：	需要销毁线程池中所有线程。
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2012-12-07	周锋	      	创建
*******************************************************************************/
CThreadP::~CThreadP(void)
{
	Clear();
}

/*******************************************************************************
* 功能描述：	启动一个线程。
* 输入参数：	pfnWork	-- 线程的执行函数；
* 输出参数：	
* 返 回 值：	返回线程状态句柄，如该句柄可Wait则说明线程已经停止。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2012-12-07	周锋	      	创建
*******************************************************************************/
HANDLE CThreadP::BeginTread(AFX_THREADPROC pfnWork, LPVOID lpContext)
{
	//如果有空闲线程，则利用已有空闲线程
	m_csLock.Lock();
	for (size_t i=0; i<m_vecThread.size(); i++)
	{
		ThreadInfo *pInfo = m_vecThread[i];
		if (NULL == pInfo->pfnWork)
		{
			pInfo->csLock.Lock();
			ResetEvent(pInfo->hWorking);
			pInfo->pfnWork = pfnWork;
			pInfo->lpContext = lpContext;
			pInfo->csLock.Unlock();
			m_csLock.Unlock();
			return pInfo->hWorking;
		}
	}
	m_csLock.Unlock();
 
	//如果找不到空闲的线程，则新建一个线程 
	ThreadInfo *pInfo = new ThreadInfo;
	pInfo->pThis = this;
	pInfo->hWorking = CreateEvent(NULL, TRUE, FALSE, NULL);
	pInfo->pfnWork = pfnWork;
	pInfo->lpContext = lpContext;
	pInfo->hThread = AfxBeginThread(TH_Work, pInfo, m_nPriority)->m_hThread;
	m_csLock.Lock();
	m_vecThread.push_back(pInfo);
	m_csLock.Unlock();

	return pInfo->hWorking;
}

//清理所有的线程
void CThreadP::Clear()
{
	m_bStop = true;
	for (size_t i=0; i<m_vecThread.size(); i++)
	{
		ThreadInfo *pInfo = m_vecThread[i];
		WaitForSingleObject(pInfo->hThread, INFINITE);
		delete pInfo;
	}
	m_vecThread.clear();
}

/*******************************************************************************
* 功能描述：	线程工作函数。
* 输入参数：	lpContext	-- ThreadInfo类型指针，指示出该线程信息；
* 输出参数：	
* 返 回 值：	返回0xAABB。
* 其它说明：	
* 修改日期		修改人			修改内容
* ------------------------------------------------------------------------------
* 2012-12-07	周锋	      	创建
*******************************************************************************/
UINT CThreadP::TH_Work(LPVOID lpContext)
{
	ThreadInfo *pInfo = (ThreadInfo *)lpContext;
	while (!pInfo->pThis->m_bStop)
	{
		pInfo->csLock.Lock();
		if (pInfo->pfnWork)
		{
			//任务执行过程中无需加锁(除了本线程外不可能有其它线程将pfnWork置空)
			pInfo->csLock.Unlock();
			pInfo->pfnWork(pInfo->lpContext);

			//任务结束后，将pfnWork置空
			pInfo->csLock.Lock();
			pInfo->pfnWork = NULL;
			pInfo->lpContext = NULL;
			pInfo->csLock.Unlock();
			SetEvent(pInfo->hWorking);
		}
		else
		{
			pInfo->csLock.Unlock();
		}
		Sleep(10);
	}

	pInfo->hThread = NULL;
	return 0xAABB;
}

}
