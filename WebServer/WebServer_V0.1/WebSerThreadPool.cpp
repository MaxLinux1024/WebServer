#include <stdlib.h>

#include "WebSerThreadPool.h"
#include <stdio.h>
threadpool_t* threadpool_create(int nThreadCount, int nQueueSize, int nFlags)
{
	threadpool_t* pool;
	int i;
	do{
		if(nThreadCount <=0 || nThreadCount > MAX_THREADS || nQueueSize <= 0 || nQueueSize > MAX_QUEUE)
		{
			return NULL;
		}

		if((pool = (threadpool_t* )malloc(sizeof(threadpool_t))) == NULL)
		{
			break;
		}

		pool->thread_count = 0;
		pool->queue_size = nQueueSize;
		pool->head = pool->tail = pool->count = 0;
		pool->shutdown = pool->started = 0;

		pool->threads = (pthread_t* )malloc(sizeof(pthread_t) * nThreadCount);
		pool->queue = (threadpool_task_t* )malloc(sizeof(threadpool_task_t) * nQueueSize);

		if((pthread_mutex_init(&(pool->lock),NULL) != 0) ||
			(pthread_cond_init(&(pool->notify),NULL) != 0)||
			(pool->threads == NULL) ||
			(pool->queue == NULL))
		{
			break;
		}

		for(i = 0;i < nThreadCount; i++)
		{
			if(pthread_create(&(pool->threads[i]),NULL,threadpool_thread,(void* )pool) != 0)
			{
				threadpool_destory(pool,0);
				return NULL;
			}
			pool->thread_count++;
			pool->started++;
		}

		return pool;
	}while(false);

	if(pool != NULL)
	{
		threadpool_free(pool);
	}
	
	return NULL;
}

int threadpool_add(threadpool_t * pPool, void(* function)(void *), void * pArgument, int nFlags)
{
	int err = 0;
	int next;
	if(pPool == NULL || function == NULL)
	{
		return THREADPOOL_INVALID;
	}
	if(pthread_mutex_lock(&(pPool->lock)) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}
	next = (pPool->tail + 1) % pPool->queue_size;

	do{

		if(pPool->count == pPool->queue_size)
		{
			err = THREADPOOL_QUEUE_FULL;
			break;
		}
		if(pPool->shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}
		printf("threadpool_add queue \n");
		pPool->queue[pPool->tail].function = function;
		pPool->queue[pPool->tail].argument = pArgument;
		pPool->tail = next;
		pPool->count += 1;

		if(pthread_cond_signal(&(pPool->notify)) != 0)
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}
	}while(false);
	
	if(pthread_mutex_unlock(&pPool->lock) != 0) {
		  err = THREADPOOL_LOCK_FAILURE;
	  }
	return err;
}

int threadpool_destory(threadpool_t * pPool, int nFlags)
{
	int i, err = 0;
	if(pPool == NULL)
	{
		return THREADPOOL_INVALID;
	}
	if(pthread_mutex_lock(&(pPool->lock)) != 0)
	{
		return THREADPOOL_LOCK_FAILURE;
	}

	do
	{
		if(pPool->shutdown)
		{
			err = THREADPOOL_SHUTDOWN;
			break;
		}

		pPool->shutdown = (nFlags & THREADPOOL_GRACEFUL)?
			graceful_shutdown : immediate_shutdown;

		if((pthread_cond_broadcast(&(pPool->notify)) != 0) ||
			(pthread_mutex_unlock(&(pPool->lock)) != 0 ))
		{
			err = THREADPOOL_LOCK_FAILURE;
			break;
		}

		for(i = 0;i < pPool->thread_count ; i++)
		{
			if(pthread_join(pPool->threads[i], NULL) != 0)
			{
				err = THREADPOOL_THREAD_FAILURE;
			}
		}
	}while(false);

	if(!err)
	{
		threadpool_free(pPool);
	}

	return err;
}

int threadpool_free(threadpool_t * pPool)
{
	if(pPool == NULL || pPool->started > 0)
	{
		return -1;
	}
	if(pPool->threads)
	{
		free(pPool->threads);
		free(pPool->queue);

		pthread_mutex_lock(&(pPool->lock));
		pthread_mutex_destroy(&(pPool->lock));
		pthread_cond_destroy(&(pPool->notify));
	}
	free(pPool);
	return 0;
}

static void* threadpool_thread(void * pThreadpool)
{
	printf("threadpool_thread \n");
	threadpool_t* pPool = (threadpool_t* )pThreadpool;
	threadpool_task_t task;
	for(;;)
	{
		pthread_mutex_lock(&(pPool->lock));
		printf("threadpool lock \n");

		while((pPool->count == 0) && (!pPool->shutdown))
		{
			pthread_cond_wait(&(pPool->notify),&(pPool->lock));
		}
		printf("threadpool pthread_cond_wait \n");
		if((pPool->shutdown == immediate_shutdown) ||
			((pPool->shutdown == graceful_shutdown) &&
			(pPool->count == 0)))
		{
			break;
		}
		printf("task into %p\n",pPool->queue[pPool->head].function);
		task.function = pPool->queue[pPool->head].function;
		task.argument = pPool->queue[pPool->head].argument;

		pPool->head = (pPool->head + 1) % pPool->queue_size;
		pPool->count -= 1;

		pthread_mutex_unlock(&(pPool->lock));
		(*(task.function))(task.argument);
	}
	--pPool->started;
	pthread_mutex_unlock(&(pPool->lock));
	pthread_exit(NULL);
	return NULL;
}
