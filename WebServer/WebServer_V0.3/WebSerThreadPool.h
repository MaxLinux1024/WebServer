//
// Created by Administrator on 2022/3/17.
//

#ifndef WEBSERVER_0_2_WEBSERTHREADPOOL_H
#define WEBSERVER_0_2_WEBSERTHREADPOOL_H

#include <pthread.h>
#include <functional>
#include <memory>
#include <vector>

enum THREADPOOLSTATE
{
    THREADPOOL_CREATE_SUCCESS = 0,
    THREADPOOL_GRACEFUL = 1,
    THREADPOOL_CREATE_FAIL = -1,
    THREADPOOL_INVALID = -2,
    THREADPOOL_LOCK_FAILURE = -3,
    THREADPOOL_QUEUE_FULL = -4,
    THREADPOOL_SHUTDOWN = -5,
    THREADPOOL_THREAD_FAILURE = -6
};



const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} THREADPOOL_SHUTDOWN_T;

struct ThreadPoolTask {
    std::function<void(std::shared_ptr < void > )> fun;
    std::shared_ptr<void> args;
};

void myHandler(std::shared_ptr<void> pReq);

class WebSerThreadPool {

public:
    int WebSerCreateThreadPool(int nThreadCount, int nQueueSize);
    int WebSerThreadPoolAddTask(std::shared_ptr<void> args, std::function<void(std::shared_ptr< void >)> = myHandler);
    int WebSerThreadPoolDestory();
    int WebSerThreadPoolFree();
    static void *WebSerThreadPoolThread(void *args);

public:
    WebSerThreadPool();
    ~WebSerThreadPool();

    static WebSerThreadPool* GetInstance(){
        static WebSerThreadPool Instance;
        return &Instance;
    }
public:
    pthread_mutex_t m_lock;
    pthread_cond_t m_notify;
    std::vector <pthread_t> m_verThreads;
    std::vector <ThreadPoolTask> m_verTaskQueue;
    int m_nThreadCount;
    int m_nQueueSize;
    int m_nHead;
    // tail 指向尾节点的下一节点
    int m_nTail;
    int m_nCount;
    int m_nShutDownState;
    int m_nStarted;

};


#endif //WEBSERVER_0_2_WEBSERTHREADPOOL_H
