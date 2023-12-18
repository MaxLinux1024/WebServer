//
// Created by Administrator on 2022/3/17.
//

#include "WebSerThreadPool.h"
#include "WebSerRequestData.h"

WebSerThreadPool::~WebSerThreadPool()
{
    WebSerThreadPoolDestory();
}

//void myHandler(std::shared_ptr<void> pReq)
//{
//    std::shared_ptr <WebSerRequestData> pRequestData = std::static_pointer_cast<WebSerRequestData>(pReq);
//    pRequestData->WebSerHandleRequest();
//}/

int WebSerThreadPool::WebSerCreateThreadPool(int unSize,int unMaxSize)
{

    printf("unSize:%d ,unMaxSize:%d\n",unSize,unMaxSize);
    m_unInitSize = unSize;
    m_unMaxSize = unMaxSize;
    WebSerAddThreadToPool(unSize);
}

int WebSerThreadPool::WebSerAddThreadToPool(int unSize)
{
#ifdef THREADPOOL_AUTO_GROW
    if(!m_bRun)
    {
        throw std::runtime_error("Grow on ThreadPool is stopped.");
    }
    std::unique_lock<std::mutex> mutexThreadPoolGrow{ m_mutexThreadPoolGrow };
#endif
    for(; m_vecThreadPools.size() < m_unMaxSize && unSize > 0; --unSize)
    {
        m_vecThreadPools.emplace_back([this]
        {
            while(true)
            {
                Task task;
                {
                    std::unique_lock<std::mutex> mutexTaskQueue{m_mutexTaskQueue};
                    m_conTaskQueue.wait(mutexTaskQueue);
                    #if 0
                    printf("!!!!! \n");
                    m_conTaskQueue.wait(mutexTaskQueue,[this]{
                        //printf("!!!!! \n");
                        return !m_bRun && !m_vecTaskQueue.empty();
                    });
                    printf("111111 \n");
                    #endif
                    if(!m_bRun || m_vecTaskQueue.empty())
                    {
                        continue;
                    }
                    m_nIdlThreadNum--;
                    task = std::move(m_vecTaskQueue.front());
                    m_vecTaskQueue.pop_back();
                }
                task();
#ifdef THREADPOOL_AUTO_GROW
                if (m_nIdlThreadNum > 0 && m_vecThreadPools.size() > m_unInitSize) 
                    return;
#endif // !THREADPOOL_AUTO_GROW
                {
                    std::unique_lock<std::mutex> mutexTaskQueue{m_mutexTaskQueue};
                    m_nIdlThreadNum++;
                }
            }
        }
        );
        {
            std::unique_lock<std::mutex> mutexTaskQueue{m_mutexTaskQueue};
            m_nIdlThreadNum++;
        }
        printf("m_vecThreadPools: %d \n",m_vecThreadPools.size());
    }
}

int WebSerThreadPool::WebSerThreadPoolDestory()
{
    m_bRun = false;
    m_conTaskQueue.notify_all();
    for (std::thread& thread : m_vecThreadPools) {
        if (thread.joinable())
        {
            thread.join(); 
        }
    }
    return 0;
}
