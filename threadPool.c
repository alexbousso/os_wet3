#include <pthread.h>
#include <stdlib.h>
#include "threadPool.h"
#include <assert.h>

#ifdef DEBUG_ON
#include <stdio.h>
#endif

static void print(const char *msg) {
	#ifdef DEBUG_ON
	fprintf(stderr, msg);
	fprintf(stderr, "\n");
	#endif
}

typedef struct taskToRun_t {
	void *param;
	void (*computeFunc) (void *);
} TaskToRun;

typedef enum {
	SUCCESS,
	NULL_ARGUMENT,
	QUEUE_FAILURE,
} Result;

static Result wrapperToTask(ThreadPool* threadPool) {
	if (!threadPool) {
		// mabye we should return an int for error and check in tpDestroy???
		print("ERROR: wrapperToTask() received a null argument!");
		return NULL_ARGUMENT;
	}
	
	while (!threadPool->destroyThreads) {
		TaskToRun *tsk;
		
		print("try to lock tasksQueueLock (wrapper)");
		pthread_mutex_lock(&threadPool->tasksQueueLock);

		
		print("try to lock dontAddNewTaskLock (wrapper)");
		pthread_mutex_lock(&threadPool->dontAddNewTaskLock);
		print("dontAddNewTaskLock locked (wrapper)");
		
		while (threadPool->dontAddNewTasks == 0 && osIsQueueEmpty(threadPool->waitingTasks)) {
			pthread_cond_wait(&(threadPool->DestroyIsOnOrTaskQNotEmpty), &(threadPool->tasksQueueLock));
		}
		print("tasksQueueLock locked (wrapper)");
		if (threadPool->dontAddNewTasks && osIsQueueEmpty(threadPool->waitingTasks)) {
			return SUCCESS;
		}
		
		pthread_mutex_unlock(&threadPool->dontAddNewTaskLock);
		print("dontAddNewTaskLock unlocked (wrapper)");
		
		//now: Q is not empty!
		tsk = osDequeue(threadPool->waitingTasks);
		if (!tsk) {
			// mabye we should return an int for error and check in tpDestroy???
			print("ERROR: osDequeue() returned null!");
			return QUEUE_FAILURE;
		}
		pthread_mutex_unlock(&threadPool->tasksQueueLock);
		print("tasksQueueLock unlocked (wrapper)");
		
		// Signals that the Queue is empty - for destroy.
		if(osIsQueueEmpty(threadPool->waitingTasks)){
			pthread_cond_signal(&(threadPool->TaskQueueEmpty));
			print("DestroyLock signal sent  (wrapper)");
		}
		tsk->computeFunc(tsk->param);
	}
	
	print("Quiting wrapper");
	return SUCCESS;
}

ThreadPool* tpCreate(int numOfThreads) {
	int i;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK_NP);
	
	ThreadPool *tp = malloc(sizeof(ThreadPool));
	if (!tp) {
		return NULL;
	}
	
	tp->threadsIDs = malloc(sizeof(pthread_t)*numOfThreads);
	if (!tp->threadsIDs) {
		free(tp);
		return NULL;
	}
	// Initializing fields:
	tp->numberOfThreads = numOfThreads;
	tp->dontAddNewTasks = 0;
	
	if (pthread_mutex_init(&(tp->tasksQueueLock),&mutexattr) != 0) {
		free(tp->threadsIDs);
		free(tp);
		print("ERROR: pthread_mutex_init() failed!");
		return NULL;
	}
	if (pthread_mutex_init(&(tp->DestroyLock),&mutexattr) != 0) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		free(tp->threadsIDs);
		free(tp);
		print("ERROR: pthread_mutex_init() failed!");
		return NULL;
	}
	if (pthread_mutex_init(&(tp->dontAddNewTaskLock),&mutexattr) != 0) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		pthread_mutex_destroy(&(tp->DestroyLock));
		free(tp->threadsIDs);
		free(tp);
		print("ERROR: pthread_mutex_init() failed!");
		return NULL;
	}
	
	tp->waitingTasks = osCreateQueue();
	if (!tp->waitingTasks) {
		pthread_mutex_destroy(&(tp->tasksQueueLock));
		pthread_mutex_destroy(&(tp->DestroyLock));
		pthread_mutex_destroy(&(tp->dontAddNewTaskLock));
		free(tp->threadsIDs);
		free(tp);
		print("ERROR: osCreateQueue() returned null!");
		return NULL;
	}
	//TODO: what if someone enters TaskQueue before we checked if it's legit
	
	// Creating new threads:
	for (i = 0; i < numOfThreads; i++) {
		pthread_t t;
		if (pthread_create
				(&t, NULL, (void* (*)(void *))&wrapperToTask, tp) != 0) {
			free(tp->waitingTasks);
			pthread_mutex_destroy(&(tp->tasksQueueLock));
			pthread_mutex_destroy(&(tp->DestroyLock));
			pthread_mutex_destroy(&(tp->dontAddNewTaskLock));
			free(tp->threadsIDs);
			free(tp);
			print("ERROR: pthread_create() failed!");
			return NULL;
		}
		tp->threadsIDs[i] = t;
	}
	
	return tp;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
	int i;
	if (!threadPool) return;
	
	print("try to lock dontAddNewTaskLock (tpDestroy)");
	pthread_mutex_lock(&threadPool->dontAddNewTaskLock);
	print("dontAddNewTaskLock locked  (tpDestroy)");
	
	threadPool->dontAddNewTasks = 1;
	
	pthread_mutex_unlock(&threadPool->dontAddNewTaskLock);
	print("dontAddNewTaskLock unlocked  (tpDestroy)");
	
	//if we shouldn't run all the waiting tasks in the Queue before destroy.
	if (!shouldWaitForTasks) {
	
		print("try to lock tasksQueueLock (tpDestroy)");
		pthread_mutex_lock(&threadPool->tasksQueueLock);
		print("tasksQueueLock locked  (tpDestroy)");	
		
		while(!osIsQueueEmpty(threadPool->waitingTasks)) {
			osDequeue(threadPool->waitingTasks);
		}
		
		pthread_mutex_unlock(&threadPool->tasksQueueLock);
		print("tasksQueueLock unlocked  (tpDestroy)");	
	}
	//TODO::Are we sure that we empty all the queue here?
	
	/*Checking that the task queue is empty - if we should not wait for that - we cleared the
	Queue in the last paragraph */
	
	print("try to lock DestroyLock (tpDestroy)");
	pthread_mutex_lock(&threadPool->DestroyLock);
	while (!osIsQueueEmpty(threadPool->waitingTasks)) {
		pthread_cond_wait(&(threadPool->TaskQueueEmpty),&threadPool->DestroyLock);
	}
	print("DestroyLock locked  (tpDestroy)");
		
	threadPool->destroyThreads = 1;
	pthread_mutex_unlock(&threadPool->DestroyLock);
	print("DestroyLock unlocked  (tpDestroy)");	
	
	// wait for threads to finish
	for(i=0; i<threadPool->numberOfThreads; i++){
		//Tzoof
		//This signal is sent so that the wrapper won't wait for a new task when no task will be created;
		pthread_cond_broadcast(&(threadPool->DestroyIsOnOrTaskQNotEmpty));
		print("tasksQueueLock signal sent  (tpDestroy)");
		pthread_join(threadPool->threadsIDs[i], NULL);
	}
	
	// free threadPool
	free(threadPool->waitingTasks);
	pthread_mutex_destroy(&(threadPool->tasksQueueLock));
	pthread_mutex_destroy(&(threadPool->DestroyLock));
	pthread_mutex_destroy(&(threadPool->dontAddNewTaskLock));
	free(threadPool);
	
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
		free(tsk);
		return -1;
	}
	
	print("try to lock tasksQueueLock (tpInsertTask)");
	pthread_mutex_lock(&(threadPool->tasksQueueLock));
	print("tasksQueueLock locked  (tpInsertTask)");
	
	tsk->computeFunc = computeFunc;
	tsk->param = param;
	assert(threadPool);
	assert(threadPool->waitingTasks);
	osEnqueue(threadPool->waitingTasks, tsk);
	pthread_mutex_unlock(&(threadPool->tasksQueueLock));
	print("tasksQueueLock unlocked  (tpInsertTask)");
	pthread_cond_signal(&(threadPool->DestroyIsOnOrTaskQNotEmpty));
	print("tasksQueueLock signal sent  (tpInsertTask)");
	return 0;
}





















