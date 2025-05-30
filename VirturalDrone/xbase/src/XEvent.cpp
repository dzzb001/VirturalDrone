// 2008-02-23
// XEvent.cpp
// guoshanhe
// windows下的事件概念和unix下的条件变量概念

#include "XEvent.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XEvent
////////////////////////////////////////////////////////////////////////////////
#ifdef __WINDOWS__
XEvent::XEvent(BOOL bManualReset)
	: m_bManualReset(bManualReset)
{
	m_handle = ::CreateEvent(NULL, bManualReset, FALSE, NULL);
	ASSERT(m_handle != NULL);
}

XEvent::~XEvent()
{
	BOOL ret = ::CloseHandle(m_handle);
	ASSERT(ret);
	ret = TRUE;
}

void XEvent::Set()
{
	BOOL ret = ::SetEvent(m_handle);
	ASSERT(ret);
	ret = TRUE;
}

void XEvent::Reset()
{
	if (!m_bManualReset)
	{
		ASSERT(!"this is a auto event, Cann't manual reset.");
		return;
	}
	BOOL ret = ::ResetEvent(m_handle);
	ASSERT(ret);
	ret = TRUE;
}

void XEvent::Wait()
{
	::WaitForSingleObject(m_handle, INFINITE);
}

BOOL XEvent::TryWait(uint32 uMilliseconds)
{
	if (WAIT_OBJECT_0 == ::WaitForSingleObject(m_handle, uMilliseconds))
	{
		return TRUE;
	}
	return FALSE;
}
#endif//__WINDOWS__


////////////////////////////////////////////////////////////////////////////////
// XEvent
////////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
XEvent::XEvent(BOOL bManualReset)
	: m_bManualReset(bManualReset)
{
	m_flag = FALSE;
	::pthread_mutex_init(&m_mutex, NULL);
	::pthread_cond_init(&m_handle, NULL);
}

XEvent::~XEvent()
{
	int ret = ::pthread_cond_destroy(&m_handle);
	ASSERT(!ret && "some threads are currently waiting on 'cond'.");
	ret = ::pthread_mutex_destroy(&m_mutex);
	ASSERT(!ret && "the mutex is currently locked.");
	ret = 0;
}

void XEvent::Set()
{
	::pthread_mutex_lock(&m_mutex);
	m_flag = TRUE;
	if (m_bManualReset)
		::pthread_cond_broadcast(&m_handle);
	else
		::pthread_cond_signal(&m_handle);
	::pthread_mutex_unlock(&m_mutex);
}

void XEvent::Reset()
{
	if (!m_bManualReset)
	{
		ASSERT(!"this is a auto event, Cann't manual reset.");
		return;
	}
	::pthread_mutex_lock(&m_mutex);
	m_flag = FALSE;
	::pthread_mutex_unlock(&m_mutex);
}

void XEvent::Wait()
{
	while(1)
	{
		::pthread_mutex_lock(&m_mutex);
		if (!m_flag)
		{
			::pthread_cond_wait(&m_handle, &m_mutex);
		}
		if (m_flag)
		{
			if (!m_bManualReset) m_flag = FALSE;
			::pthread_mutex_unlock(&m_mutex);
			break;
		}
		else
		{
			::pthread_mutex_unlock(&m_mutex);
			continue;
		}
	}
	return;
}

BOOL XEvent::TryWait(uint32 uMilliseconds)
{
	BOOL flag = FALSE;
	::pthread_mutex_lock(&m_mutex);
	flag = m_flag;
	if (!m_flag)
	{
		struct timeval tv = {};
		gettimeofday(&tv, NULL);

		uint32 tv_sec	= tv.tv_sec + uMilliseconds / 1000;
		uint32 tv_usec	= tv.tv_usec + (uMilliseconds % 1000 * 1000);
		tv_sec += tv_usec / 1000000;
		tv_usec %= 1000000;

		struct timespec abstime = {0};
		abstime.tv_sec  = tv_sec;
		abstime.tv_nsec = tv_usec * 1000;

		::pthread_cond_timedwait(&m_handle, &m_mutex, &abstime);
		flag = m_flag;
	}
	if (!m_bManualReset) m_flag = FALSE;
	::pthread_mutex_unlock(&m_mutex);

	return flag;
}
#endif//__GNUC__

} // namespace xbase

