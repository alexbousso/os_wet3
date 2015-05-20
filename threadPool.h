#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>
#include "osqueue.h"

#define DEBUG_ON // TODO: Remove!!!

typedef struct thread_pool {
	// The number of threads when ThreadPool is created
	int numberOfThreads;
	
	//An array of all the ID of the threads
	thread_t* threadsIDs;
	
	// A queue of tasks waiting to be inserted in a thread
	OSQueue *waitingTasks;
	
	// A flag that signals the threads that tpDestroy() has been called
	int destroyThreads;
	
	// After calling tpDestroy we will turn on this flag which will prevent tpInsertTask to work.  
	int dontAddNewTasks;
	
	//Lock for the queue waitingTasks
	pthread_mutex_t tasksQueueLock;
	
	//lock for the destroy function - send signal when Queue is empty and we can Destroy
	pthread_mutex_t DestroyLock;
	
	//Condition that sends signal when the Task you is not empty 
	pthread_cond_t DestroyIsOnOrTaskQNotEmpty;
	
	//Condition that sends signal when the Task you is empty 
	pthread_cond_t TaskQueueEmpty;
} ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
