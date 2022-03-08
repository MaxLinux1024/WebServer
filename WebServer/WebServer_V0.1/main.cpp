#include <queue>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <signal.h>

#include "WebSerRequestData.h"
#include "WebSerEpoll.h"
#include "WebSerThreadPool.h"
#include "util.h"

const int THREADPOOL_THREAD_NUM = 4;
const int QUEUE_SIZE = 65535;

const int PORT =8888;

const std::string PATH = "/";

const int TIMER_TIME_OUT = 500;

extern pthread_mutex_t qlock;

void WebServerSegerror( int nsig )
{
	//DA_LOGDBG("%s got segment error !", "sdkdevds");
	//CSddsServer::SetTskState( );
}
void WebServerPipeerror( int nsig )
{
	//DA_LOGDBG("%s got pipe error !", "sdkdevds");
}

void WebServerTerminate( int nsig )
{
	//DA_LOGDBG("%s got User1Signal  !", "sdkdevds");
	//CSddsServer::SetTskState( );
	return ;
}


void HandleSigpipe()
{
	struct sigaction sa;
	sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);	
	sigaddset(&sa.sa_mask, SIGPIPE);    	
	sigaddset(&sa.sa_mask, SIGTERM);	
	sigaddset(&sa.sa_mask, SIGINT);	 

	sa.sa_handler = WebServerPipeerror;   
	sigaction(SIGPIPE, &sa, NULL);    	
	sa.sa_handler = WebServerTerminate;    
	sigaction(SIGTERM, &sa, NULL);		
	sa.sa_handler = WebServerTerminate;    
	sigaction(SIGINT, &sa, NULL);
}

void acceptConnection(int listen_fd,int epoll_fd,const std::string& path);
extern std::priority_queue<mytimer*, std::deque<mytimer*>, timerCmp> myTimerQueue;


void myHandler(void *args)
{
    requestData *req_data = (requestData*)args;
	printf("req_data fd %d,%p \n",req_data->fd,req_data);
	if(req_data != NULL)
		printf("req_data is null \n");
    req_data->handleRequest();
}

void acceptConnection(int listen_fd, int epoll_fd, const std::string &path)
{
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	socklen_t client_addr_len = 0;
	int accept_fd = 0;
    while((accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0)
    {
		int ret = setSocketNonBlocking(accept_fd);
        if (ret < 0)
        {
            perror("Set non block failed!");
            return;
        }

		printf("accept fd：%d \n",accept_fd);
		printf("epoll_fd fd：%d \n",epoll_fd);
        requestData *req_info = new requestData(epoll_fd, accept_fd, path);
		printf("req_info：%p \n",req_info);
		
        // 文件描述符可以读，边缘触发(Edge Triggered)模式，保证一个socket连接在任一时刻只被一个线程处理
        __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
        WebSerEpoll::GetInstance()->WebSerEpollAdd(epoll_fd, accept_fd, static_cast<void*>(req_info), _epo_event);
        // 新增时间信息
        mytimer *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
        req_info->addTimer(mtimer);
        pthread_mutex_lock(&qlock);
        myTimerQueue.push(mtimer);
        pthread_mutex_unlock(&qlock);
	}
}

void handle_events(int epoll_fd,int listen_fd,epoll_event*events, int events_num, const std::string &path, threadpool_t* tp )
{
	for(int i = 0;i < events_num;i++)
	{
		//获取事件的描述符
		requestData* request = (requestData*)(events[i].data.ptr);
        int fd = request->getFd();

        // 有事件发生的描述符为监听描述符
        if(fd == listen_fd)
        {
            std::cout << "This is listen_fd" << std::endl;
            acceptConnection(listen_fd, epoll_fd, path);
        }
        else
        {
            // 排除错误事件
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (!(events[i].events & EPOLLIN)))
            {
                printf("error event\n");
                delete request;
                continue;
            }
			printf("deal event \n");
            // 将请求任务加入到线程池中
            // 加入线程池之前将Timer和request分离
            request->seperateTimer();
			printf("data %p\n",events[i].data.ptr);
            int rc = threadpool_add(tp, myHandler, events[i].data.ptr, 0);
			printf("threadpool add :%d ,%p\n",rc,myHandler);
			if(tp == NULL)
				printf("tp is NULL");
			if(myHandler == NULL)
				printf("myHandler is NULL");
        }
	}
}

int socket_bind_listen(int port)
{
	if(port < 1024 || port > 65535)
		return -1;

	int listen_fd = -1;
	if((listen_fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
		return -1;


	//
	int optval = 1;
	if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		return -1;

	struct sockaddr_in server_addr;
	bzero((char*)&server_addr,sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons((unsigned short)port);
	if(bind(listen_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
		return -1;

	 // 开始监听，最大等待队列长为LISTENQ
    if(listen(listen_fd, LISTENNUMBER) == -1)
        return -1;

    // 无效监听描述符
    if(listen_fd == -1)
    {
        close(listen_fd);
        return -1;
    }

    return listen_fd;	
}

void handle_expired_event()
{
    pthread_mutex_lock(&qlock);
    while (!myTimerQueue.empty())
    {
        mytimer *ptimer_now = myTimerQueue.top();
        if (ptimer_now->isDeleted())
        {
            myTimerQueue.pop();
            delete ptimer_now;
        }
        else if (ptimer_now->isvalid() == false)
        {
            myTimerQueue.pop();
            delete ptimer_now;
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&qlock);
}

int main()
{
	
	HandleSigpipe();
	WebSerEpoll::GetInstance()->WebSerEpollInit();
	int epoll_fd = WebSerEpoll::GetInstance()->WebSerGetEpollFd();
	if(epoll_fd < 0)
	{
		printf("epoll create error");
		return -1;
	}
	printf("epoll create:%d \n",epoll_fd);
	threadpool_t* threadpool = threadpool_create(THREADPOOL_THREAD_NUM, QUEUE_SIZE, 0);
	if(threadpool == NULL)
		printf("create threadpool error \n");
	int listen_fd = socket_bind_listen(PORT);
    if (listen_fd < 0) 
    {
        perror("socket bind failed");
        return 1;
    }
	printf("listen fd:%d \n",listen_fd);
    if (setSocketNonBlocking(listen_fd) < 0)
    {
        perror("set socket non block failed");
        return 1;
    }

	__uint32_t event = EPOLLIN | EPOLLET;
	requestData* req = new(requestData);
	printf("req %p \n",req);
	req->setFd(listen_fd);
	//struct epoll_event* events= WebSerEpoll::GetInstance()->WebSerGetEpollEvent();
	WebSerEpoll::GetInstance()->WebSerEpollAdd(epoll_fd, listen_fd, static_cast<void*>(req), event);
	while(true)
	{
	
		int events_num = WebSerEpoll::GetInstance()->WebSerEpollWaitCount();
		if(events_num == 0)
			continue;
		struct epoll_event* events= WebSerEpoll::GetInstance()->WebSerGetEpollEvent();
		handle_events(epoll_fd, listen_fd, events, events_num, PATH, threadpool);
		handle_expired_event();
		
	}
	return 0;
}

