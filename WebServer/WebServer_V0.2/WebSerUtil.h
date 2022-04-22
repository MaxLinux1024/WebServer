//
// Created by Administrator on 2022/3/22.
//

#ifndef WEBSERVER_0_2_WEBSERUTIL_H
#define WEBSERVER_0_2_WEBSERUTIL_H


#include <cstdlib>

ssize_t readn(int fd, void *buff, size_t n);

ssize_t writen(int fd, void *buff, size_t n);

void handle_for_sigpipe();

int SetSocketNonBlocking(int nFd);


#endif //WEBSERVER_0_2_WEBSERUTIL_H
