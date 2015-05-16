#include <pthread.h>
#include <stdlib.h>
#include "threadPool.h"

#ifdef DEBUG_ON
#include <stdio.h>
#endif

typedef struct taskToRun_t {
	void *param;
	void (*computeFunc) (void *);
} TaskToRun;

typedef struct result_t {
	SUCCESS,
	NULL_ARGUMENT,
	QUEUE_FAILURE,
} Result;

Result wrapperToTask(ThreadPool* threadPool) {
	if (!threadPool) {
		// mabye we should return an int for error and check in tpDestroy???
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: wrapperToTask() received a null argument!\n");
		#endif
		return NULL_ARGUMENT;
	}
	
	while (tp->destroyThreads == 0) {
		TaskToRun *tsk;
		
		// condition
		while (osIsQueueEmpty(&(threadPool->waitingTasks))) {
			// wait();
		}
		// LOCK!!!
		tsk = osDequeue(threadPool.waitingTasks);
		if (!tsk) {
			// mabye we should return an int for error and check in tpDestroy???
			#ifdef DEBUG_ON
			fprintf(stderr, "ERROR: osDequeue() returned null!\n");
			#endif
			return QUEUE_FAILURE;
		}
		tsk->computeFunc(tsk->param);
		// UNLOCK
	}
	
	return SUCCESS;
}

ThreadPool* tpCreate(int numOfThreads) {
	int i;
	ThreadPool *tp = malloc(sizeof(ThreadPool));
	if (!tp) {
		return NULL;
	}
	
	// Initializing fields:
	tp->numberOfThreads = numOfThreads;
	tp->dontAddNewTasks = 0;
	
	if (pthread_mutex_init(&(tp->tasksQueueLock),
			PTHREAD_MUTEX_ERRORCHECK) != 0) {
		free(tp);
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: pthread_mutex_init() failed!\n");
		#endif
		return NULL;
	}
	
	tp->waitingTasks = osCreateQueue();
	if (!tp->waitingTasks) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		free(tp);
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: osCreateQueue() returned null!\n");
		#endif
		return NULL;
	}
	
	// Creating new threads:
	for (i = 0; i < numOfThreads; i++) {
		thread *t;
		if (pthread_create(t, NULL, &wrappperToTask, tp) != 0) {
			free(tp->waitingTasks);
			pthread_mutex_destroy(&(tp->tasksQueueLock));
			free(tp);
			#ifdef DEBUG_ON
			fprintf(stderr, "ERROR: pthread_create() failed!\n");
			#endif
			return NULL;
		}
	}
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
	if (!threadPool) return;
	
	threadPool->dontAddNewTasks = 1;
	if (shouldWaitForTasks != 0) {
		// wait for enqueued tasks
	}
	
	// wait for threads to finish
	// check Result
	
	// free threadPool
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *),
		void* param) {
	TaskToRun *tsk = malloc(sizeof(TaskToRun));
	if (!tsk) return -1;
	
	if (!threadPool || !computeFunc) {
		free(tsk);
		return -1;
	}
	
	if (threadPool->dontAddNewTasks) {
		return -1;
	}
	
	pthread_mutex_lock(&(threadPool->tasksQueueLock));
	tsk->computeFunc = computeFunc;
	tsk->param = param;
	osEnqueue(&(threadPool->waitingTasks), tsk);
	pthread_mutex_unlock(&(threadPool->tasksQueueLock));
	
	return 0;
}





















