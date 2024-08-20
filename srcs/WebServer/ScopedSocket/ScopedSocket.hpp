#pragma once 

#include <unistd.h>


// DONT ADD THE NONBLOCK FLAG TO THE PROXY SOCKETS so false it
class ScopedSocket {
public:
    ScopedSocket(int fd = -1, int socket_flags = 0);
    ~ScopedSocket();

    ScopedSocket(const ScopedSocket&) = delete;           
    ScopedSocket& operator=(const ScopedSocket&) = delete;

    ScopedSocket(ScopedSocket&& other) noexcept;   
    ScopedSocket& operator=(ScopedSocket&& other) noexcept;

    int     get(void) const;
    void    reset(int fd = -1);
    int     release(void);

protected:
    int     _fd;
    void    setSocketFlags(int flags);
};
