/*******************************************************************************

    File : Mutex.cpp

*******************************************************************************/
#include "Mutex.h"


//namespace common {
////////////////////////////////////////////////////////////////////////////////
// Mutex
////////////////////////////////////////////////////////////////////////////////
Mutex::Mutex()
{
//    pthread_mutexattr_t attr= {PTHREAD_MUTEX_RECURSIVE_NP};
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mutex);
}

void Mutex::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

void Mutex::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
}


////////////////////////////////////////////////////////////////////////////////
// MutexLocker
////////////////////////////////////////////////////////////////////////////////
MutexLocker::MutexLocker(Mutex* pMutex) : m_pMutex(pMutex)
{
    if(m_pMutex != NULL)
        m_pMutex->Lock();
}

MutexLocker::~MutexLocker()
{
    if(m_pMutex != NULL)
        m_pMutex->Unlock();
}

void MutexLocker::Lock()
{
    if(m_pMutex != NULL)
        m_pMutex->Lock();
}

void MutexLocker::Unlock()
{
    if(m_pMutex != NULL)
        m_pMutex->Unlock();
}

//} // namespace common
