#ifndef __WEBSEReEPOLL_H_
#define __WEBSEReEPOLL_H_

#include <sys/epoll.h>

class WebSerEpoll{
public:
	static WebSerEpoll* GetInstance(){
		static WebSerEpoll instance;
		return &instance;
	}

	int WebSerEpollInit();
	int WebSerEpollResource();
	int WebSerGetEpollFd();
	
	int WebSerEpollAdd(int nEpollFd,int nFd, void* pRequest, __uint32_t events);
	int WebSerEpollMod(int nEpollFd,int nFd, void* pRequest, __uint32_t events);
	int WebSerEpollDelete(int nEpollFd,int nFd, void* pRequest, __uint32_t events);
	int	WebSerEpollWaitCount(int nEpollFd,int nFd, void* pRequest, __uint32_t events);
	
private:
	WebSerEpoll() = delete;
	~WebSerEpoll();
private:
	int m_nEpollFd;
	struct epoll_event* events;

};

#endif
