#ifndef __UTIL_H_
#define __UTIL_H_

#include <cstdlib>

ssize_t readn(int fd, void* buff, size_t n);
ssize_t writen(int fd,void* buff, size_t n);
int setSocketNonBlocking(int fd);

#endif
