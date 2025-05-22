// 2009-07-27
// guoshanhe
// XAtomic.h
// ԭ�Ӳ�����

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
	XAtomic& operator ++();		// ++��ǰ
	XAtomic& operator --();		// -- ��ǰ
	void	 operator ++(int);	// ++�ں�
	void	 operator --(int);	// --�ں�
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
	XMutex		m_lock;	// linux����ʱû�и��õİ취,������(ԭ�Ӳ���ֻ�������ں�)
#endif//__GNUC__
};

}//namespace xbase

using namespace xbase;

#endif//_X_ATOMIC_H_
