// 2008-02-23
// guoshanhe
// XCritical.h
// 临界区类(允许递归锁)

#pragma once

#ifndef _X_CRITICAL_SECTION_H_
#define _X_CRITICAL_SECTION_H_

#include "XDefine.h"
#include "XMutex.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XCritical
////////////////////////////////////////////////////////////////////////////////
class XCritical
{
public:
	XCritical();

	~XCritical();

public:
	void Lock();

	BOOL TryLock();

	void UnLock();

private:
	XCritical(const XCritical&);
	XCritical& operator=(const XCritical&);

private:
#ifdef __WINDOWS__
	CRITICAL_SECTION m_sCriticalSection;
#endif//__WINDOWS__
#ifdef __GNUC__
	pthread_mutex_t		m_mutex;
#endif//__GNUC__
};

} // namespace xbase

using namespace xbase;

#endif//_X_CRITICAL_SECTION_H_
