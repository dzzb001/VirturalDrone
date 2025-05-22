/*******************************************************************************
* ��Ȩ���� (C) ���������ں��Ƽ����޹�˾ 2012
* 
* �ļ����ƣ� 	ThreadP.h
* �ļ���ʶ�� 	// �����ù���ƻ���
* ����ժҪ�� 	�Զ����̳߳��ࡣ
* ����˵���� 	// �������ݵ�˵��
* ��ǰ�汾�� 	V1.0
* ��    �ߣ� 	�ܷ�
* ������ڣ� 	2012-12-07
*******************************************************************************/
#pragma once
#include <vector>
#include <afxmt.h>

namespace Tool
{

class CThreadP
{
public:
	//����
	CThreadP(int nPriority);

	//����s
	~CThreadP(void);

	//����һ���߳�
	HANDLE BeginTread(AFX_THREADPROC pfnWork, LPVOID lpContext);

	//�������е��߳�
	void Clear();

private:

	struct ThreadInfo
	{
		CThreadP *pThis;	//Thisָ��
		HANDLE hWorking;	//��ǰ����״̬(���Wait�ɹ�˵���߳��Ѿ�����)
		HANDLE hThread;		//��ǰ�߳̾��
		AFX_THREADPROC pfnWork;		//��������
		LPVOID lpContext;			//��������
		CCriticalSection csLock;	//���ݱ�����
	};

	//�߳��б�
	std::vector<ThreadInfo *> m_vecThread;
	CCriticalSection m_csLock;

	//�߳̿��Ʊ�־λ
	volatile bool m_bStop;

	//�߳����ȼ�
	int m_nPriority;

	//ʵ���̺߳���
	static UINT TH_Work(LPVOID lpContext);
};


//Ԥ�������̳߳�
extern CThreadP TP1;		//��ͨ�߳����ȼ�
extern CThreadP TP2;		//�߼��߳����ȼ�
extern CThreadP TP3;		//���(ʵʱ)�߳����ȼ�

}
