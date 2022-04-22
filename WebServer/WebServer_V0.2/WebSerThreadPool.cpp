//
// Created by Administrator on 2022/3/17.
//

#include "WebSerThreadPool.h"
#include "WebSerRequestData.h"

WebSerThreadPool::WebSerThreadPool()
{
    m_lock = PTHREAD_MUTEX_INITIALIZER;
    m_notify = PTHREAD_COND_INITIALIZER;
    m_nThreadCount = 0;
    m_nQueueSize = 0;
    m_nHead = 0;
    m_nTail = 0;
    m_nCount = 0;
    m_nShutDownState = 0;
    m_nStarted = 0;
}

WebSerThreadPool::~WebSerThreadPool()
{

}

void myHandler(std::shared_ptr<void> pReq)
{
    std::shared_ptr <WebSerRequestData> pRequestData = std::static_pointer_cast<WebSerRequestData>(pReq);
    pRequestData->WebSerHandleRequest();
}

int WebSerThreadPool::WebSerCreateThreadPool(int nThreadCount, int nQueueSize)
{
    bool bErr = false;
    do{
        if(nThreadCount <= 0 || nThreadCount > MAX_THREADS || nQueueSize <= 0 || nQueueSize > MAX_QUEUE)
        {
            nThreadCount = 4;
            nQueueSize = 1024;
        }
        m_nThreadCount = 0;
        m_nQueueSize = nQueueSize;
        m_nHead = m_nTail = m_nCount = 0;
        m_nShutDownState = m_nStarted = 0;

        m_verThreads.resize(nThreadCount);
        m_verTaskQueue.resize(nQueueSize);

        for(int i = 0; i < nThreadCount; ++i)
        {
            if(pthread_create(&m_verThreads[i], NULL, WebSerThreadPoolThread, this) != 0)
            {
                return THREADPOOL_CREATE_FAIL;
            }
            ++m_nThreadCount;
            ++m_nStarted;
        }
    }while(false);

    return THREADPOOL_CREATE_SUCCESS;
}

int WebSerThreadPool::WebSerThreadPoolAddTask(std::shared_ptr<void> args, std::function<void(std::shared_ptr< void >)> func )
{
    int nNext, err = 0;
    if (pthread_mutex_lock(&m_lock) != 0) {
        return THREADPOOL_LOCK_FAILURE;
    }
    do {
        nNext = (m_nTail + 1) % m_nQueueSize;
        // 队列满
        if (m_nCount == m_nQueueSize) {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        // 已关闭
        if (m_nShutDownState) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        m_verTaskQueue[m_nTail].fun = func;
        m_verTaskQueue[m_nTail].args = args;
        m_nTail = nNext;
        ++m_nCount;

        /* pthread_cond_broadcast */
        if (pthread_cond_signal(&m_notify) != 0) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while (false);

    if (pthread_mutex_unlock(&m_lock) != 0)
        err = THREADPOOL_LOCK_FAILURE;
    return err;
}

int WebSerThreadPool::WebSerThreadPoolDestory()
{

}

int WebSerThreadPool::WebSerThreadPoolFree()
{

}

void *WebSerThreadPool::WebSerThreadPoolThread(void *args)
{
    while (true) {
        ThreadPoolTask task;
        WebSerThreadPool* pWebSerThreadPool = static_cast<WebSerThreadPool*> (args);
        pthread_mutex_lock(&(pWebSerThreadPool->m_lock));
        while (((pWebSerThreadPool->m_nCount) == 0) && (!(pWebSerThreadPool->m_nShutDownState))) {
            pthread_cond_wait(&(pWebSerThreadPool->m_notify), &(pWebSerThreadPool->m_lock));
        }
        if ((pWebSerThreadPool->m_nShutDownState == immediate_shutdown) ||
            ((pWebSerThreadPool->m_nShutDownState == graceful_shutdown) && (pWebSerThreadPool-> m_nCount == 0))) {
            break;
        }
        task.fun = pWebSerThreadPool->m_verTaskQueue[pWebSerThreadPool->m_nHead].fun;
        task.args = pWebSerThreadPool->m_verTaskQueue[pWebSerThreadPool->m_nHead].args;
        pWebSerThreadPool->m_verTaskQueue[pWebSerThreadPool->m_nHead].fun = nullptr;
        pWebSerThreadPool->m_verTaskQueue[pWebSerThreadPool->m_nHead].args.reset();
        pWebSerThreadPool->m_nHead = (pWebSerThreadPool->m_nHead + 1) % pWebSerThreadPool->m_nQueueSize;
        --(pWebSerThreadPool->m_nCount);
        pthread_mutex_unlock(&(pWebSerThreadPool->m_lock));
        (task.fun)(task.args);
    }

    WebSerThreadPool* pWebSerThreadPool = static_cast<WebSerThreadPool*> (args);
    --(pWebSerThreadPool->m_nStarted);

    pthread_mutex_unlock(&(pWebSerThreadPool->m_lock));
    pthread_exit(NULL);
    return (NULL);
}