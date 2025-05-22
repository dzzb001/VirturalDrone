// 2008-02-23 15:50
// Xmutex.h
// guoshanhe
// ������(������ݹ���)

#pragma once

#ifndef _X_MUTEX_H_
#define _X_MUTEX_H_

#include "XDefine.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XMutex
////////////////////////////////////////////////////////////////////////////////
class XMutex
{
public:
	XMutex();

	~XMutex();

public:
	void Lock();

	BOOL TryLock();

	void UnLock();

private:
	XMutex(const XMutex&);
	XMutex& operator=(const XMutex&);

private:
#ifdef __WINDOWS__
	HANDLE				m_mutex;
#endif//__WINDOWS__
#ifdef __GNUC__
	pthread_mutex_t		m_mutex;
#endif//__GNUC__
};


////////////////////////////////////////////////////////////////////////////////
// XLockGuard
////////////////////////////////////////////////////////////////////////////////
template<class XLock>
class XLockGuard
{
public:
	XLockGuard(XLock& lock)
		:m_lock(lock)
	{
		m_lock.Lock();
	}

	~XLockGuard()
	{
		m_lock.UnLock();
	}

private:
	XLock& m_lock;
};

} // namespace xbase

using namespace xbase;

#endif//_X_MUTEX_H_
