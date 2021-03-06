#include "WebSerEpoll.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

WebSerEpoll::~WebSerEpoll()
{
	WebSerEpollResource();
}

int WebSerEpoll::WebSerEpollInit()
{
	m_nEpollFd = epoll_create(1);
	printf("WebSerEpoll:%d \n",m_nEpollFd);
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
	if(epoll_ctl(nEpollFd,EPOLL_CTL_ADD,nFd,&event) < 0)
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
	if(epoll_ctl(nEpollFd,EPOLL_CTL_MOD,nFd,&event) < 0)
	{
		perror("epoll mod error");
		return -1;
	}
	return 0;
	
}

int WebSerEpoll::WebSerEpollDelete(int nEpollFd,int nFd, void* pRequest, __uint32_t events)
{
	struct epoll_event event;
	event.data.ptr = pRequest;
	event.events = events;
	if(epoll_ctl(nEpollFd,EPOLL_CTL_DEL,nFd,&event) < 0)
	{
		perror("epoll add error");
		return -1;
	}
	return 0;
}

int WebSerEpoll::WebSerEpollWaitCount()
{
	int nCount = epoll_wait(m_nEpollFd, events, MAXEVENTS, 500);
	if(nCount < 0 )
	{
		perror("epoll wait error");
	}
	return nCount;
}
