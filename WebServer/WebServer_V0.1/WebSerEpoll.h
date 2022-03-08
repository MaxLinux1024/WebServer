#ifndef __WEBSEReEPOLL_H_
#define __WEBSEReEPOLL_H_

#include <sys/epoll.h>

const int LISTENNUMBER = 1024;
const int MAXEVENTS = 5000;

class WebSerEpoll{
public:

	~WebSerEpoll();
	
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
	int	WebSerEpollWaitCount();
	epoll_event* WebSerGetEpollEvent();

private:
	int m_nEpollFd;
	struct epoll_event* events;

};

#endif
