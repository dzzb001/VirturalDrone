// 2008-02-23 17:06
// XThread.h
// guoshanhe
// �߳���ʵ��


#pragma once

#ifndef _X_THREAD_H_
#define _X_THREAD_H_

#include "XDefine.h"
#include "XEvent.h"

namespace xbase {

#ifdef __WINDOWS__
	typedef unsigned (XAPI *PFTHREADPROC)(void* pParam);
#endif//__WINDOWS__
#ifdef __GNUC__
	typedef void* (XAPI *PFTHREADPROC)(void* pParam);
#endif//__GNUC__

///////////////////////////////////////////////////////////////////////////////
// XThread
////////////////////////////////////////////////////////////////////////////////
class XThread
{
public:
	XThread();

	virtual ~XThread();

public:
	virtual BOOL Start(PFTHREADPROC pfThreadProc = NULL, void *pParam = NULL);

	// ����˳��ź�
	virtual BOOL TryWaitQuit(uint32 uMilliseconds = 0);

	// ����ֹͣ����
	virtual void PostStop();

	// �ȴ��߳̽���,������Դ
	virtual void Join();

	// ɱ���߳�(������ʹ��,��ʹ��PostStop��֪ͨ�߳̽���)
	void Kill();

	// ��ȡ�߳�ID
	uint32 GetThreadID() { return m_uThreadID; };

protected:
	virtual void Entry() = 0;

	// �߳����ѽ���(������ʹ��,��ֱ��return�˳��߳�ִ�к���)
	void Exit();

private:
	XThread(const XThread &);
	XThread& operator=(const XThread &);

private:
#ifdef __WINDOWS__
	static unsigned XAPI _Entry(void* pParam);
	HANDLE				m_hThread;
#endif//__WINDOWS__

#ifdef __GNUC__
	static void* XAPI _Entry(void *pParam);
#endif//__GNUC__

private:
	uint32				m_uThreadID;
	PFTHREADPROC		m_pfThreadProc;
	void				*m_pParam;

	XEvent				m_evQuit;		// �������ʹ���ֹ������¼�
};

// �߳�����ָ������ֵ
void X_Sleep(uint32 nMillisecond);

// ��ȡ�߳�����ID
uint32 X_GetThreadID(void);

} // namespace xbase

using namespace xbase;

#endif//_X_THREAD_H_
