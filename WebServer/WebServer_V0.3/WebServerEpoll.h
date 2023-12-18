//
// Created by Administrator on 2022/3/17.
//

#ifndef WEBSERVER_0_2_WEBSERVEREPOLL_H
#define WEBSERVER_0_2_WEBSERVEREPOLL_H

#include "WebSerRequestData.h"
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <memory>

//void myHandlerNew(std::shared_ptr<void> req)
//{
//    std::shared_ptr<WebSerRequestData> request = std::static_pointer_cast<WebSerRequestData>(req);
//    request->WebSerHandleRequest();
//}

class WebServerEpoll {
public:
    WebServerEpoll() = default;
    ~WebServerEpoll();

    static WebServerEpoll* GetInstance(){
        static WebServerEpoll instance;
        return &instance;
    }

public:

    int WebSerEpollInit(int nMaxEvents,int nListenNum);
    int WebSerEpollAdd(int nFd,std::shared_ptr<WebSerRequestData> pRequestData, __uint32_t events);
    int WebSerEpollMod(int nFd,std::shared_ptr<WebSerRequestData> pRequestData, __uint32_t events);
    int WebSerEpollDel(int nFd, __uint32_t events);
    void WebSerEpollWait(int nListenFd, int nMaxEvents, int nTimeOut);
    std::vector<std::shared_ptr<WebSerRequestData>>WebSerGetEventsRequest(int nListenFd, int nEventsNum, std::string strPath);
    void WebSerAcceptConnection( int nListenFd, std::string strPath);
private:
    epoll_event* m_pEvents;
    std::unordered_map<int, std::shared_ptr<WebSerRequestData>> m_mapRequestDataInfo;
    int m_nEpollFd;
    std::string m_strPath;
};


#endif //WEBSERVER_0_2_WEBSERVEREPOLL_H
