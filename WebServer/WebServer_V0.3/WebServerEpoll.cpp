//
// Created by Administrator on 2022/3/17.
//
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include "WebServerEpoll.h"
#include "WebSerThreadPool.h"
#include "WebSerUtil.h"
#include "WebServerDefine.h"

void myHandler1()
{
    printf("num:1111 \n");
}

void myHandler(std::shared_ptr<void> req)
{
    std::shared_ptr<WebSerRequestData> request = std::static_pointer_cast<WebSerRequestData>(req);
    request->WebSerHandleRequest();
}
WebServerEpoll::~WebServerEpoll()
{

}

int WebServerEpoll::WebSerEpollInit(int nMaxEvents,int nListenNum)
{
    if((nListenNum <= 0) || nListenNum <= 0)
        return -1;
    m_nEpollFd = epoll_create(nListenNum);
    if(m_nEpollFd == -1)
    {
        return -1;
    }
    m_pEvents = new epoll_event[nMaxEvents];
    return 0;
}

int WebServerEpoll::WebSerEpollAdd(int nFd,std::shared_ptr<WebSerRequestData> pRequestData, __uint32_t events)
{
    struct epoll_event event;
    event.data.fd = nFd;
    event.events = events;
    if(epoll_ctl(m_nEpollFd, EPOLL_CTL_ADD, nFd, &event) < 0)
    {
        perror("epoll add error");
        return -1;
    }
    m_mapRequestDataInfo[nFd] = pRequestData;
    return 0;
}

int WebServerEpoll::WebSerEpollMod(int nFd,std::shared_ptr<WebSerRequestData> pRequestData, __uint32_t events)
{
    struct epoll_event event;
    event.data.fd = nFd;
    event.events = events;
    if(epoll_ctl(m_nEpollFd, EPOLL_CTL_MOD, nFd, &event) < 0)
    {
        perror("epoll mod error");
        return -1;
    }
    m_mapRequestDataInfo[nFd] = pRequestData;
    return 0;
}
int WebServerEpoll::WebSerEpollDel(int nFd, __uint32_t events)
{
    struct epoll_event event;
    event.data.fd = nFd;
    event.events = events;
    if(epoll_ctl(m_nEpollFd, EPOLL_CTL_DEL, nFd, &event) < 0)
    {
        perror("epoll del error");
        return -1;
    }
    auto mapRequestInfoIter = m_mapRequestDataInfo.find(nFd);
    if(mapRequestInfoIter != m_mapRequestDataInfo.end())
    {
        m_mapRequestDataInfo.erase(nFd);
        return 0;
    }
    return -1;
}
void WebServerEpoll::WebSerEpollWait(int nListenFd, int nMaxEvents, int nTimeOut)
{
    int nEventCount = epoll_wait(m_nEpollFd, m_pEvents, nMaxEvents,nTimeOut);
    if(nEventCount < 0)
    {
        return;
    }
    std::vector<std::shared_ptr<WebSerRequestData>> pRequestData = WebSerGetEventsRequest(nListenFd,nEventCount, m_strPath);
    if(pRequestData.size() > 0)
    {
        for(auto& RequestData:pRequestData)
        {
            WebSerThreadPool::GetInstance()->WebSerThreadPoolAddTask(myHandler,RequestData);
            //WebSerThreadPool::GetInstance()->WebSerThreadPoolAddTask1(myHandler1);
        }
    }
}

std::vector<std::shared_ptr<WebSerRequestData>>WebServerEpoll::WebSerGetEventsRequest(int nListenFd, int nEventsNum, std::string strPath)
{
    std::vector<std::shared_ptr<WebSerRequestData>> vecRequestData;
    for(int i = 0; i < nEventsNum; ++i)
    {
        int fd = m_pEvents[i].data.fd;

        if(fd == nListenFd)
        {
            WebSerAcceptConnection(nListenFd, strPath);
        }
        else if(fd < 3) {
            printf("WebSerAcceptConnection  fd error\n");
            break;
        }
        else
        {
            if((m_pEvents[i].events & EPOLLERR) ||
            (m_pEvents[i].events & EPOLLHUP)||
            (!(m_pEvents[i].events & EPOLLIN)))
            {
                printf("fd error \n");
                auto mapRequestDataIter = m_mapRequestDataInfo.find(fd);
                if(mapRequestDataIter != m_mapRequestDataInfo.end())
                {
                    m_mapRequestDataInfo.erase(fd);
                }

                continue;
            }
            /*
            printf("m_pEvents[i].data.fd\n");
            if(m_pEvents[i].data.fd == fd) {
                std::shared_ptr <WebSerRequestData> pRequestDataInfo(m_mapRequestDataInfo[fd]);
                //pRequestDataInfo->WebSerSeperateTimer();
                vecRequestData.push_back(pRequestDataInfo);
                auto mapRequestDataIter = m_mapRequestDataInfo.find(fd);
                if (mapRequestDataIter != m_mapRequestDataInfo.end()) {
                    m_mapRequestDataInfo.erase(fd);
                }
            }
            else
                continue;
            */


            auto mapRequestDataIter = m_mapRequestDataInfo.find(fd);
            if(mapRequestDataIter != m_mapRequestDataInfo.end())
            {
                std::shared_ptr<WebSerRequestData> pRequestDataInfo(m_mapRequestDataInfo[fd]);
                vecRequestData.push_back(pRequestDataInfo);
                pRequestDataInfo->WebSerSeperateTimer();
                m_mapRequestDataInfo.erase(fd);
                //WebServerEpoll::GetInstance()->WebSerEpollDel(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
            }
            else {
                continue;
            }
        }
    }
    return vecRequestData;
}

void WebServerEpoll::WebSerAcceptConnection( int nListenFd, std::string strPath)
{
    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    socklen_t nClientAddrLen = 0;
    int nAcceptFd = 0;
    while((nAcceptFd = accept(nListenFd, (struct sockaddr *)&clientaddr, &nClientAddrLen)) > 0)
    {
        //std::cout << inet_ntoa(clientaddr.sin_addr) << std::endl;
        //std::cout << ntohs(clientaddr.sin_port) << std::endl;

        int nRet = SetSocketNonBlocking(nAcceptFd);
        if(nRet < 0)
        {
            perror("Set non block failed");
            return;
        }
        std::shared_ptr<WebSerRequestData> pRequestData(new WebSerRequestData(m_nEpollFd,nAcceptFd,strPath));
        if(pRequestData != nullptr)
        {
            __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
            int nRet = WebSerEpollAdd(nAcceptFd, pRequestData, _epo_event);
            // 新增时间信息
            if(nRet == -1)
                continue;

            std::shared_ptr <MyTimer> pTimer(new MyTimer(pRequestData, TIMER_TIME_OUT));
            if(pTimer != nullptr)
            {
                pRequestData->WebSerAddTimer(pTimer);
                MutexLockGuard lock;
                WebSerMimeType::GetInstance()->m_myTimerQueue.push(pTimer);
            }
            else
                WebServerEpoll::GetInstance()->WebSerEpollDel(pRequestData->WebSerGetFd(), EPOLLIN | EPOLLET | EPOLLONESHOT);

        }
    }

}