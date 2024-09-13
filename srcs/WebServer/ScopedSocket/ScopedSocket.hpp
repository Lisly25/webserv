#pragma once 

#include <unistd.h>


// DONT ADD THE NONBLOCK FLAG TO THE PROXY SOCKETS so false it
class ScopedSocket
{
public:
    ScopedSocket(int fd = -1, int socket_flags = 0);
    virtual ~ScopedSocket();

    ScopedSocket(const ScopedSocket&) = delete;           
    ScopedSocket& operator=(const ScopedSocket&) = delete;

    ScopedSocket(ScopedSocket&& other) noexcept;   
    ScopedSocket& operator=(ScopedSocket&& other) noexcept;

    int     getFd(void) const;
    void    reset(int fd = -1);
    int     release(bool closeSocket = true);
    void    setOwnership(bool ownsSocket);

protected:
    int     _fd;
    void    setSocketFlags(int flags);
     bool    _ownsSocket;
};
