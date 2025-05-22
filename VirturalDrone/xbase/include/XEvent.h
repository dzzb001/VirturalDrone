// 2008-02-23 17:21
// XEvent.h
// guoshanhe
// windows�µ��¼������unix�µ�������������

#pragma once

#ifndef _X_EVENT_H_
#define _X_EVENT_H_

#include "XDefine.h"
#include "XCritical.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XEvent
////////////////////////////////////////////////////////////////////////////////
class XEvent
{
public:
	// Ĭ��Ϊ�Զ�������
	XEvent(BOOL bManualReset = FALSE);

	~XEvent();

public:
	void Set();

	void Reset();

	void Wait();

	BOOL TryWait(uint32 uMilliseconds = 0);

private:
	XEvent(const XEvent &);
	XEvent& operator=(const XEvent&);

private:
#ifdef __WINDOWS__
	HANDLE				m_handle;
#endif//__WINDOWS__
#ifdef __GNUC__
	pthread_cond_t		m_handle;			// ��������
	volatile BOOL		m_flag;				// �ű�
	pthread_mutex_t		m_mutex;			// ��
#endif//__GNUC__

	BOOL				m_bManualReset;		// �Ƿ��ֹ�����
};

} // namespace xbase

using namespace xbase;

#endif//_X_EVENT_H_
