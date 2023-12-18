#include "WebSerRequestData.h"
#include "WebServerEpoll.h"
#include "WebSerThreadPool.h"
#include "WebSerUtil.h"
#include <sys/epoll.h>
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
#include <memory>


static const int MAXEVENTS = 5000;
static const int LISTENQ = 1024;
const int THREADPOOL_THREAD_NUM = 10;
const int QUEUE_SIZE = 65535;

const int PORT = 80;
const int ASK_STATIC_FILE = 1;
const int ASK_IMAGE_STITCH = 2;

const std::string PATH = "/";

const int TIMER_TIME_OUT = 500;

int SocketBindListen(int nPort)
{
    //if(nPort < 1024 || nPort > 65535)
     //   return -1;

    int nListenFd = socket(AF_INET,SOCK_STREAM,0);
    if(nListenFd == -1)
        return -1;

    int nOptVal = -1;
    if(setsockopt(nListenFd, SOL_SOCKET, SO_REUSEADDR, &nOptVal, sizeof(nOptVal)) == -1)
        return -1;



    struct sockaddr_in server_addr;
    bzero((char *)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)nPort);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(nListenFd,(struct sockaddr *)&server_addr,sizeof(server_addr)) == -1)
        return -1;

    if(listen(nListenFd,LISTENQ) == -1)
        return -1;

    if (nListenFd == -1) {
        close(nListenFd);
        return -1;
    }
    return nListenFd;
}

int main(int argc,char* argv[]) {
    handle_for_sigpipe();
    int nThreadPool;
    if(argc != 1)
    {
        nThreadPool = atoi(argv[1]);
    }
    else
    {
        nThreadPool = THREADPOOL_THREAD_NUM;
    }
    printf("pool :%d \n",nThreadPool);
    if (WebServerEpoll::GetInstance()->WebSerEpollInit(MAXEVENTS, LISTENQ) < 0) {
        perror("epoll init failed");
        return 1;
    }
    if (WebSerThreadPool::GetInstance()->WebSerCreateThreadPool(nThreadPool, 32) < 0) {
    //if (WebSerThreadPool::GetInstance()->WebSerCreateThreadPool() < 0) {
        printf("Threadpool create failed\n");
        return 1;
    }
    int nListenFd = SocketBindListen(8888);
    //printf("listen:%d \n",nListenFd);
    if (nListenFd < 0) {
        perror("socket bind failed");
        return 1;
    }
    if (SetSocketNonBlocking(nListenFd) < 0) {
        perror("set socket non block failed");
        return 1;
    }
    std::shared_ptr <WebSerRequestData> pRequest(new WebSerRequestData());
    pRequest->WebSerSetFd(nListenFd);
    if (WebServerEpoll::GetInstance()->WebSerEpollAdd(nListenFd, pRequest, EPOLLIN | EPOLLET) < 0) {
        perror("epoll add error");
        return 1;
    }
    while (true) {
        WebServerEpoll::GetInstance()->WebSerEpollWait(nListenFd, MAXEVENTS, -1);
        WebSerMimeType::GetInstance()->WebSerHandleExpiredEvent();
    }
    return 0;
}
