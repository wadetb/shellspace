#include "common.h"
#include "thread.h"


pthread_mutex_t s_mutex[MUTEX_COUNT];


void Thread_Init()
{
	int 	err;
	uint 	mutexIter;

	for ( mutexIter = 0; mutexIter < MUTEX_COUNT; mutexIter++ )
	{
		err = pthread_mutex_init( &s_mutex[mutexIter], NULL );
		if ( err != 0 )
			FAIL( "Thread_Init: pthread_mutex_init returned %i", err );
	}
}


void Thread_Shutdown()
{
	int 	err;
	uint 	mutexIter;

	for ( mutexIter = 0; mutexIter < MUTEX_COUNT; mutexIter++ )
	{
		err = pthread_mutex_destroy( &s_mutex[mutexIter] );
		if ( err != 0 )
			FAIL( "Thread_Shutdown: pthread_mutex_destroy returned %i", err );
	}
}


void Thread_Lock( EMutex mutex )
{
	pthread_mutex_lock( &s_mutex[mutex] );
}


void Thread_Unlock( EMutex mutex )
{
	pthread_mutex_unlock( &s_mutex[mutex] );
}


Thread_ScopeLock::Thread_ScopeLock( EMutex mutex ) : 
	lockedMutex( mutex )
{
	Thread_Lock( mutex );
}


Thread_ScopeLock::~Thread_ScopeLock()
{
	Thread_Unlock( lockedMutex );
}


void Thread_Sleep( uint ms )
{
	struct timespec tim;
	struct timespec tim2;

	tim.tv_sec  = 0;
	tim.tv_nsec = ms * 1000000;
	
	nanosleep( &tim, &tim2 );
}
