#ifndef __THREAD_H__
#define __THREAD_H__

enum EMutex
{
	MUTEX_API,
	MUTEX_INQUEUE,
	MUTEX_COUNT
};

enum EThread
{
	THREAD_MAIN,
	THREAD_COUNT
};

void Thread_Init();
void Thread_Shutdown();

void Thread_Lock( EMutex mutex );
void Thread_Unlock( EMutex mutex );

struct Thread_ScopeLock
{
	EMutex lockedMutex;
	Thread_ScopeLock( EMutex mutex );
	~Thread_ScopeLock();
};

void Thread_Sleep( uint ms );

#endif