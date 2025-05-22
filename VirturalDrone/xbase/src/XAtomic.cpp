// 2009-07-27
// guoshanhe
// XAtomic.cpp
// 原子操作类

#include "XAtomic.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XAtomic
////////////////////////////////////////////////////////////////////////////////
XAtomic::XAtomic(int nValue/* = 0*/)
	: m_counter(nValue)
{
	// empty
}

XAtomic::~XAtomic()
{
	// empty
}

XAtomic::operator int()
{
	return (int)m_counter;
}

#ifdef __WIN32__

XAtomic& XAtomic::operator =(int nValue)
{
	LONG tmp = (LONG)nValue;
	InterlockedExchange(&m_counter, tmp);
	return *this;
}

XAtomic& XAtomic::operator +=(int nValue)
{
	LONG tmp = (LONG)nValue;
	InterlockedExchangeAdd(&m_counter, tmp);
	return *this;
}

XAtomic& XAtomic::operator -=(int nValue)
{
	LONG tmp = (LONG)nValue * (-1);
	InterlockedExchangeAdd(&m_counter, tmp);
	return *this;
}

XAtomic& XAtomic::operator ++()
{
	InterlockedIncrement(&m_counter);
	return *this;
}

XAtomic& XAtomic::operator --()
{
	InterlockedDecrement(&m_counter);
	return *this;
}

#endif//__WIN32__

#ifdef __WIN64__

XAtomic& XAtomic::operator =(int nValue)
{
	LONG64 tmp = (LONG64)nValue;
	InterlockedExchange64(&m_counter, tmp);
	return *this;
}


XAtomic& XAtomic::operator +=(int nValue)
{
	LONG64 tmp = (LONG64)nValue;
	InterlockedExchangeAdd64(&m_counter, tmp);
	return *this;
}

XAtomic& XAtomic::operator -=(int nValue)
{
	LONG64 tmp = (LONG64)nValue * (-1);
	InterlockedExchangeAdd64(&m_counter, tmp);
	return *this;
}

XAtomic& XAtomic::operator ++()
{
	InterlockedIncrement64(&m_counter);
	return *this;
}

XAtomic& XAtomic::operator --()
{
	InterlockedDecrement64(&m_counter);
	return *this;
}

#endif//__WIN64__

#ifdef __GNUC__

XAtomic& XAtomic::operator =(int nValue)
{
	XLockGuard<XMutex> lock(m_lock);
	m_counter = nValue;
	return *this;
}


XAtomic& XAtomic::operator +=(int nValue)
{
	XLockGuard<XMutex> lock(m_lock);
	m_counter += nValue;
	return *this;
}

XAtomic& XAtomic::operator -=(int nValue)
{
	XLockGuard<XMutex> lock(m_lock);
	m_counter -= nValue;
	return *this;
}

XAtomic& XAtomic::operator ++()
{
	XLockGuard<XMutex> lock(m_lock);
	m_counter++;
	return *this;
}

XAtomic& XAtomic::operator --()
{
	XLockGuard<XMutex> lock(m_lock);
	m_counter--;
	return *this;
}

#endif//__GNUC__

void XAtomic::operator ++(int)
{
	++*this;
	return;
}

void XAtomic::operator --(int)
{
	--*this;
	return;
}

bool XAtomic::operator ==(int nValue)
{
	int tmp = (int)m_counter;
	return tmp == nValue;
}

bool XAtomic::operator !=(int nValue)
{
	int tmp = (int)m_counter;
	return tmp != nValue;
}

bool XAtomic::operator >=(int nValue)
{
	int tmp = (int)m_counter;
	return tmp >= nValue;
}

bool XAtomic::operator <=(int nValue)
{
	int tmp = (int)m_counter;
	return tmp <= nValue;
}

bool XAtomic::operator >(int nValue)
{
	int tmp = (int)m_counter;
	return tmp > nValue;
}

bool XAtomic::operator <(int nValue)
{
	int tmp = (int)m_counter;
	return tmp < nValue;
}

}//namespace xbase
