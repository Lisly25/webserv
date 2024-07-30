#pragma once 

#include <unistd.h>

class ScopedSocket {
public:
    ScopedSocket(int fd = -1);
    ~ScopedSocket();
    ScopedSocket(const ScopedSocket&) = delete;
    ScopedSocket& operator=(const ScopedSocket&) = delete;

    int     get(void) const;
    void    reset(int fd = -1);
    int     release(void);


private:
    int     _fd;
};
