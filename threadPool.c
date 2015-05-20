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
		
		pthread_mutex_lock(&threadPool->tasksQueueLock);
		// condition
		// Tzoof
		while (osIsQueueEmpty(&(threadPool->waitingTasks))) {
			pthread_cond_wait(&(threadPool->DestroyIsOnOrTaskQNotEmpty), &(threadPool->tasksQueueLock) );
		}
		//This means Destroy is on - the condition was sent from destroy
		if(osIsQueueEmpty(&(threadPool->waitingTasks)) && tp->dontAddNewTasks){
				return SUCCESS;
		}
		
		tsk = osDequeue(threadPool.waitingTasks);
		if (!tsk) {
			// mabye we should return an int for error and check in tpDestroy???
			#ifdef DEBUG_ON
			fprintf(stderr, "ERROR: osDequeue() returned null!\n");
			#endif
			return QUEUE_FAILURE;
		}
		pthread_mutex_unlock(&threadPool->tasksQueueLock);
		//Tzoof 
		// Signals that the Queue is empty - for destroy.
		if(!osIsQueueEmpty(&(threadPool->waitingTasks))){
			pthread_cond_signal(&(threadPool->TaskQueueEmpty));
		}
		tsk->computeFunc(tsk->param);
	}
	
	return SUCCESS;
}

ThreadPool* tpCreate(int numOfThreads) {
	int i;

	ThreadPool *tp = malloc(sizeof(ThreadPool));
	if (!tp) {
		return NULL;
	}
	tp->threadsIDs = malloc(sizeof(thread_t)*numOfThreads);
	if (!tp->threadsIDs) {
		free(tp);
		return NULL;
	}
	// Initializing fields:
	tp->numberOfThreads = numOfThreads;
	tp->dontAddNewTasks = 0;
	
	if (pthread_mutex_init(&(tp->tasksQueueLock),
			PTHREAD_MUTEX_ERRORCHECK) != 0) {
		free(tp->threadsIDs);
		free(tp);
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: pthread_mutex_init() failed!\n");
		#endif
		return NULL;
	}
	if (pthread_mutex_init(&(tp->DestroyLock),
			PTHREAD_MUTEX_ERRORCHECK) != 0) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		free(tp->threadsIDs);
		free(tp);
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: pthread_mutex_init() failed!\n");
		#endif
		return NULL;
	}
	
	tp->waitingTasks = osCreateQueue();
	if (!tp->waitingTasks) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		pthread_mutex_destroy(&(tp->DestroyLock));
		free(tp->threadsIDs);
		free(tp);
		#ifdef DEBUG_ON
		fprintf(stderr, "ERROR: osCreateQueue() returned null!\n");
		#endif
		return NULL;
	}
	//TODO: what if someone enters TaskQueue before we checked if it's legit
	
	// Creating new threads:
	for (i = 0; i < numOfThreads; i++) {
		thread_t t;
		if (pthread_create(&t, NULL, &wrappperToTask, tp) != 0) {
			free(tp->waitingTasks);
			pthread_mutex_destroy(&(tp->tasksQueueLock));
			pthread_mutex_destroy(&(tp->DestroyLock));
			free(tp->threadsIDs);
			free(tp);
			#ifdef DEBUG_ON
			fprintf(stderr, "ERROR: pthread_create() failed!\n");
			#endif
			return NULL;
		}
		threadsIDs[i]=t;
	}
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
	int i;
	if (!threadPool) return;
	
	threadPool->dontAddNewTasks = 1;
	
	//if we shouldn't run all the waiting tasks in the Queue before destroy.
	if (!shouldWaitForTasks) {
		pthread_mutex_lock(&threadPool->tasksQueueLock);
		while(!osIsQueueEmptytheadPool->waitingTasks){
			osDequeue(osIsQueueEmptytheadPool->waitingTasks);
		}
		pthread_mutex_unlock(&threadPool->tasksQueueLock);
	}
	//TODO::Are we sure that we empty all the queue here?
	
	/*Checking that the task queue is empty - if we should not wait for that - we cleared the
	Queue in the last paragraph */
	pthread_mutex_lock(&threadPool->DestroyLock);
	while (!osIsQueueEmpty(&(threadPool->waitingTasks))) {
		pthread_cond_wait(&(threadPool->TaskQueueEmpty),&threadPool->DestroyLock);
	}
	threadPool->destroyThreads = 1;
	pthread_mutex_unlock(&threadPool->DestroyLock);

	
	// wait for threads to finish
	for(i=0; i<theadPool->numOfThreads; i++){
		//Tzoof
		//This signal is sent so that the wrapper won't wait for a new task when no task will be created;
		pthread_cond_signal(&(threadPool->DestroyIsOnOrTaskQNotEmpty));
		pthread_join(threadsIDs[i], NULL);
	}
	
	// free threadPool
	free(threadPool->waitingTasks);
	pthread_mutex_destroy(&(threadPool->tasksQueueLock));
	free(theadPool);
	
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
	pthread_cond_signal(&(threadPool->DestroyIsOnOrTaskQNotEmpty));
	
	return 0;
}





















