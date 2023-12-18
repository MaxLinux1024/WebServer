//
// Created by Administrator on 2022/3/17.
//

#ifndef WEBSERVER_0_2_WEBSERTHREADPOOL_H
#define WEBSERVER_0_2_WEBSERTHREADPOOL_H

#include <vector>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>
#include <thread>
#include <functional>
#include <stdexcept>

#include <pthread.h>
#include <functional>
#include <memory>
#include <vector>

#define  THREADPOOL_MAX_NUM 16


struct ThreadPoolTask {
    std::function<void(std::shared_ptr < void > )> fun;
    std::shared_ptr<void> args;
};

//void myHandler(std::shared_ptr<void> pReq);

class WebSerThreadPool {

public:
    int WebSerCreateThreadPool(int unSize = 4,int m_unMaxSize = 12);
    int WebSerAddThreadToPool(int unSize);
    
    template<class F, class... Args>
    void WebSerThreadPoolAddTask(F&& f, Args&&... args)
    {
        if (!m_bRun)
        {
            throw std::runtime_error("commit on ThreadPool is stopped.");
        }

        using RetType = decltype(f(args...)); 
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        ); 
        //std::future<RetType> future = task->get_future();
        {    // 添加任务到队列
            std::lock_guard<std::mutex> mutexTaskQueue{ m_mutexTaskQueue };
            m_vecTaskQueue.emplace_back([task]() 
            {
                (*task)();
            });
        }
#ifdef THREADPOOL_AUTO_GROW
        if (m_nIdlThreadNum < 1 && m_vecThreadPools.size() < m_unMaxSize)
            WebSerAddThreadToPool(1);
#endif // !THREADPOOL_AUTO_GROW
        m_conTaskQueue.notify_one(); 

    }

	template <class F>
	void WebSerThreadPoolAddTask1(F&& task)
	{
		if (!m_bRun) return;
		{
			std::lock_guard<std::mutex> lock{ m_mutexTaskQueue };
			m_vecTaskQueue.emplace_back(std::forward<F>(task));
            printf("WebSerThreadPoolAddTask1 m_vecTaskQueue:%d \n",m_vecTaskQueue.size());
		}
#ifdef THREADPOOL_AUTO_GROW
		if (m_nIdlThreadNum < 1 && m_vecThreadPools.size() < m_unMaxSize)
			WebSerAddThreadToPool(1);
#endif // !THREADPOOL_AUTO_GROW
		m_conTaskQueue.notify_one();
	}
    
    int WebSerThreadPoolDestory();

public:
    WebSerThreadPool() = default;
    ~WebSerThreadPool();

    static WebSerThreadPool* GetInstance(){
        static WebSerThreadPool Instance;
        return &Instance;
    }
public:
    int m_unInitSize;
    int m_unMaxSize;
    using Task = std::function<void()>;
    std::vector <std::thread> m_vecThreadPools;
    std::vector <Task> m_vecTaskQueue;
    std::mutex  m_mutexTaskQueue;
#ifdef THREADPOOL_AUTO_GROW
    std::mutex m_mutexThreadPoolGrow;
#endif //!THREADPOOL_AUTO_GROW
    std::condition_variable m_conTaskQueue;
    std::atomic<bool> m_bRun{true};
    std::atomic<int>  m_nIdlThreadNum{0};
    std::atomic<int>  m_nTaskNum{0};

};


#endif //WEBSERVER_0_2_WEBSERTHREADPOOL_H
