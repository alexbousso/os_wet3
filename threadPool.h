#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"

#define DEBUG_ON // TODO: Switch to zero

typedef struct thread_pool {
	// The number of threads when ThreadPool is created
	int numberOfThreads;
	
	// A queue of tasks waiting to be inserted in a thread
	OSQueue *waitingTasks;
	
	// A flag that signals the threads that tpDestroy() has been called
	int destroyThreads;
	// After calling tpDestroy we will turn on this flag which will prevent tpInsertTask to work.  
	int dontAddNewTasks;
	
} ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
