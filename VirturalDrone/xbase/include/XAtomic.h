// 2009-07-27
// guoshanhe
// XAtomic.h
// 原子操作类

#pragma once

#ifndef _X_ATOMIC_H_
#define _X_ATOMIC_H_

#include "XDefine.h"
#include "XMutex.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XAtomic
////////////////////////////////////////////////////////////////////////////////
class XAtomic
{
public:
	XAtomic(int nValue = 0);
	~XAtomic();

public:
	operator int();
	XAtomic& operator =(int nValue);
	XAtomic& operator +=(int nValue);
	XAtomic& operator -=(int nValue);
	XAtomic& operator ++();		// ++在前
	XAtomic& operator --();		// -- 在前
	void	 operator ++(int);	// ++在后
	void	 operator --(int);	// --在后
	bool	 operator ==(int nValue);
	bool	 operator !=(int nValue);
	bool	 operator >=(int nValue);
	bool	 operator <=(int nValue);
	bool	 operator >(int nValue);
	bool	 operator <(int nValue);

private:
	XAtomic(const XAtomic&);
	XAtomic& operator=(const XAtomic&);

private:
#ifdef __WINDOWS__
#ifdef __WIN32__
	LONG		m_counter;
#else//__WIN32__
	LONG64		m_counter;
#endif//__WIN32__
#endif//__WINDOWS__
#ifdef __GNUC__
	int			m_counter;
	XMutex		m_lock;	// linux下暂时没有更好的办法,先用锁(原子操作只让用于内核)
#endif//__GNUC__
};

}//namespace xbase

using namespace xbase;

#endif//_X_ATOMIC_H_
