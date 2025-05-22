#include "stdafx.h"
#include "ThreadP.h"
  
namespace Tool
{

//Ԥ�õ������̳߳�
CThreadP TP1(THREAD_PRIORITY_NORMAL);
CThreadP TP2(THREAD_PRIORITY_HIGHEST);
CThreadP TP3(THREAD_PRIORITY_TIME_CRITICAL);

/*******************************************************************************
* ����������	���캯����
* ���������	nPriority	-- �߳����ȼ����������AfxBeginThread��ͬ��
* ���������	
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2012-12-07	�ܷ�	      	����
*******************************************************************************/
CThreadP::CThreadP(int nPriority) : m_nPriority(nPriority)
{
	m_bStop = false;
}

/*******************************************************************************
* ����������	����������
* ����˵����	��Ҫ�����̳߳��������̡߳�
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2012-12-07	�ܷ�	      	����
*******************************************************************************/
CThreadP::~CThreadP(void)
{
	Clear();
}

/*******************************************************************************
* ����������	����һ���̡߳�
* ���������	pfnWork	-- �̵߳�ִ�к�����
* ���������	
* �� �� ֵ��	�����߳�״̬�������þ����Wait��˵���߳��Ѿ�ֹͣ��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2012-12-07	�ܷ�	      	����
*******************************************************************************/
HANDLE CThreadP::BeginTread(AFX_THREADPROC pfnWork, LPVOID lpContext)
{
	//����п����̣߳����������п����߳�
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
 
	//����Ҳ������е��̣߳����½�һ���߳� 
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

//�������е��߳�
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
* ����������	�̹߳���������
* ���������	lpContext	-- ThreadInfo����ָ�룬ָʾ�����߳���Ϣ��
* ���������	
* �� �� ֵ��	����0xAABB��
* ����˵����	
* �޸�����		�޸���			�޸�����
* ------------------------------------------------------------------------------
* 2012-12-07	�ܷ�	      	����
*******************************************************************************/
UINT CThreadP::TH_Work(LPVOID lpContext)
{
	ThreadInfo *pInfo = (ThreadInfo *)lpContext;
	while (!pInfo->pThis->m_bStop)
	{
		pInfo->csLock.Lock();
		if (pInfo->pfnWork)
		{
			//����ִ�й������������(���˱��߳��ⲻ�����������߳̽�pfnWork�ÿ�)
			pInfo->csLock.Unlock();
			pInfo->pfnWork(pInfo->lpContext);

			//��������󣬽�pfnWork�ÿ�
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
