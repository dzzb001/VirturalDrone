// 2008-02-27
// XSemaphore.h
// guoshanhe
// –≈∫≈¡ø¿‡


#include "XSemaphore.h"

namespace xbase {

////////////////////////////////////////////////////////////////////////////////
// XSemaphore
////////////////////////////////////////////////////////////////////////////////
#ifdef __WINDOWS__
XSemaphore::XSemaphore(uint32 nInitCount, uint32 nMaxCount)
	: m_counter((int)nInitCount)
{
	m_hSemaphore = CreateSemaphore(NULL, nInitCount, nMaxCount, NULL);
	ASSERT(m_hSemaphore);
}

XSemaphore::~XSemaphore()
{
	BOOL ret = CloseHandle(m_hSemaphore);
	ASSERT(ret);
	ret = 0;
}

void XSemaphore::Wait()
{
	WaitForSingleObject(m_hSemaphore, INFINITE);
	--m_counter;
}

BOOL XSemaphore::TryWait(uint32 nMillisecond)
{
	if (WaitForSingleObject(m_hSemaphore, nMillisecond) == WAIT_OBJECT_0)
	{
		--m_counter;
		return TRUE;
	}
	return FALSE;
}

BOOL XSemaphore::Post(uint32 nCount)
{
	BOOL bRet = FALSE;
	uint32 uOldCount = 0;
	bRet = ReleaseSemaphore(m_hSemaphore, nCount, (LPLONG)&uOldCount);
	if (bRet)
	{
		m_counter += (int)nCount;
		return TRUE;
	}
	return FALSE;
}

uint32 XSemaphore::GetCount()
{
	return (int)m_counter;
}
#endif//__WINDOWS__


////////////////////////////////////////////////////////////////////////////////
// XSemaphore
////////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
XSemaphore::XSemaphore(uint32 nInitCount, uint32 nMaxCount)
{
	int ret = sem_init(&m_hSemaphore, 0, nInitCount);
	ASSERT(0 == ret);
	ret = 0;
}

XSemaphore::~XSemaphore()
{
	int ret = sem_destroy(&m_hSemaphore);
	ASSERT(!ret && "some threads are currently blocked waiting on the semaphore.");
	ret = 0;
}

void XSemaphore::Wait()
{
	sem_wait(&m_hSemaphore);
}

BOOL XSemaphore::TryWait(uint32 nMillisecond)
{
	return (BOOL)!sem_trywait(&m_hSemaphore);
}

BOOL XSemaphore::Post(uint32 nCount)
{
	while (nCount--)
	{
		if (0 != sem_post(&m_hSemaphore))
		{
			return FALSE;
		}
	}
	return TRUE;
}

uint32 XSemaphore::GetCount()
{
	int count = 0;
	int ret = sem_getvalue(&m_hSemaphore, &count);
	if (!ret)
	{
		return (uint32)count;
	}
	return 0;
}
#endif//__GNUC__

} // namespace xbase

