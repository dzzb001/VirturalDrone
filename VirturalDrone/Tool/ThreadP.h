/*******************************************************************************
* 版权所有 (C) 北京正安融翰科技有限公司 2012
* 
* 文件名称： 	ThreadP.h
* 文件标识： 	// 见配置管理计划书
* 内容摘要： 	自定义线程池类。
* 其它说明： 	// 其它内容的说明
* 当前版本： 	V1.0
* 作    者： 	周锋
* 完成日期： 	2012-12-07
*******************************************************************************/
#pragma once
#include <vector>
#include <afxmt.h>

namespace Tool
{

class CThreadP
{
public:
	//构造
	CThreadP(int nPriority);

	//析构s
	~CThreadP(void);

	//启动一个线程
	HANDLE BeginTread(AFX_THREADPROC pfnWork, LPVOID lpContext);

	//清理所有的线程
	void Clear();

private:

	struct ThreadInfo
	{
		CThreadP *pThis;	//This指针
		HANDLE hWorking;	//当前工作状态(如可Wait成功说明线程已经结束)
		HANDLE hThread;		//当前线程句柄
		AFX_THREADPROC pfnWork;		//工作函数
		LPVOID lpContext;			//环境变量
		CCriticalSection csLock;	//数据保护锁
	};

	//线程列表
	std::vector<ThreadInfo *> m_vecThread;
	CCriticalSection m_csLock;

	//线程控制标志位
	volatile bool m_bStop;

	//线程优先级
	int m_nPriority;

	//实际线程函数
	static UINT TH_Work(LPVOID lpContext);
};


//预设三个线程池
extern CThreadP TP1;		//普通线程优先级
extern CThreadP TP2;		//高级线程优先级
extern CThreadP TP3;		//最高(实时)线程优先级

}
