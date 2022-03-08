#ifndef __WEBSERTHREADPOOL_H_
#define __WEBSERTHREADPOOL_H_

#include <pthread.h>

const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

typedef enum
{
	immediate_shutdown = 1,
	graceful_shutdown = 2
}threadpool_shutdown_t;

typedef struct{
	void (*function)(void*);
	void* argument;
}threadpool_task_t;

struct threadpool_t
{
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t* threads;
	threadpool_task_t* queue;
	int thread_count;
	int queue_size;
	int head;
	int tail;
	int count;
	int shutdown;
	int started;
};

threadpool_t* threadpool_create(int nThreadCount,int nQueueSize,int nFlags);
int threadpool_add(threadpool_t* pPool,void (*function)(void*),void* pArgument,int nFlags);
int threadpool_destory(threadpool_t* pPool,int nFlags);
int threadpool_free(threadpool_t* pPool);
static void* threadpool_thread(void* pThreadpool);
#endif