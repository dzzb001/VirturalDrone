/*******************************************************************************

    File : Mutex.h
    
*******************************************************************************/
#ifndef MEGA_COMMON_MUTEX_H
#define MEGA_COMMON_MUTEX_H

#include <pthread.h>
#include <stdio.h>


//namespace common {
class Condition;
////////////////////////////////////////////////////////////////////////////////
// Mutex
////////////////////////////////////////////////////////////////////////////////
class Mutex
{
public :
    Mutex();
    ~Mutex();
    void Lock();
    void Unlock();
    pthread_mutex_t* GetMutex() {
    	return &m_mutex;
    }
    
private :
    pthread_mutex_t     m_mutex;
    friend class Condition;
};


////////////////////////////////////////////////////////////////////////////////
// MutexLocker
////////////////////////////////////////////////////////////////////////////////
class MutexLocker
{
public :
    MutexLocker(Mutex* pMutex);
    ~MutexLocker();
    void Lock();
    void Unlock();

private :
    Mutex*              m_pMutex;
};

//} // namespace common
#endif
