#include "WebSerEpoll.h"

const int LISTENNUMBER = 1024;
const int MAXEVENTS = 5000;

WebSerEpoll::~WebSerEpoll()
{
	WebSerEpollResource();
}

WebSerEpoll::WebSerEpollInit()
{
	int m_nEpollFd = epoll_create(1);
	if(m_nEpollFd == -1)
		return -1;
	events = new epoll_event[MAXEVENTS];
	return 0;
}

int WebSerEpoll::WebSerEpollResource()
{
	int nEpollFd = WebSerGetEpollFd();
	if(nEpollFd != -1)
		close(nEpollFd);
	
	if(events != nullptr)
		delete events;
}

int WebSerEpoll::WebSerGetEpollFd()
{
	return m_nEpollFd;
}

epoll_event* WebSerEpoll::WebSerGetEpollEvent()
{
	return events;
}

int WebSerEpoll::WebSerEpollAdd(int nEpollFd,int nFd, void* pRequest, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = pRequest;
	event.events = events;
	if(epoll_ctl(nEpollFd,EPOLL_CTL_ADD,nFd,&events) < 0)
	{
		perror("epoll add error");
		return -1;
	}
	return 0;
}

int WebSerEpoll::WebSerEpollMod(int nEpollFd,int nFd, void* pRequest, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = pRequest;
	event.events = events;
	if(epoll_ctl(nEpollFd,EPOLL_CTL_MOD,nFd,&events) < 0)
	{
		perror("epoll add error");
		return -1;
	}
	return 0;
	
}

int WebSerEpoll::WebSerEpollDelete(int nEpollFd,int nFd, void* pRequest, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = pRequest;
	event.events = events;
	if(epoll_ctl(nEpollFd,EPOLL_CTL_DEL,nFd,&events) < 0)
	{
		perror("epoll add error");
		return -1;
	}
	return 0;
}

int WebSerEpoll::WebSerEpollWaitCount()
{
	int nCount = epoll_wait(nEpollFd, events, MAXEVENTS, timeout);
	if(nRet < 0 )
	{
		perror("epoll wait error");
	}
	return nCount;
}