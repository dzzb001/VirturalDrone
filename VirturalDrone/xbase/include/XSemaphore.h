// 2008-02-27 10:56
// XSemaphore.h
// guoshanhe
// 信号量类


#pragma once

#ifndef _X_SEMAPHORE_H_
#define _X_SEMAPHORE_H_

#include "XDefine.h"
#include "XAtomic.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XSemaphore
////////////////////////////////////////////////////////////////////////////////
class XSemaphore
{
public:
	XSemaphore(uint32 nInitCount = 0, uint32 nMaxCount = 0X7FFFFFFF);

	~XSemaphore();

	void Wait();

	BOOL TryWait(uint32 nMillisecond = 0); // Linux下该方法总是马上返回

	BOOL Post(uint32 nCount = 1);

	uint32 GetCount();

private:
	XSemaphore(const XSemaphore&);
	XSemaphore& operator=(const XSemaphore&);

private:
#ifdef __WINDOWS__
	HANDLE				m_hSemaphore;
	XAtomic				m_counter;
#endif//__WINDOWS__

#ifdef __GNUC__
	sem_t		m_hSemaphore;
#endif//__GNUC__
};

} // namespace xbase

using namespace xbase;

#endif//_X_SEMAPHORE_H_

