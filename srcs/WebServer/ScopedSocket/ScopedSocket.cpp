#include "ScopedSocket.hpp"

ScopedSocket::ScopedSocket(int fd) : _fd(fd) {}

ScopedSocket::~ScopedSocket()
{ 
    if (_fd != -1)
        close(_fd); 
}

int ScopedSocket::get() const 
{ 
    return _fd; 
}

void ScopedSocket::reset(int fd)
{
    if (_fd != -1)
        close(_fd);
    _fd = fd;
}

int ScopedSocket::release()
{
    int tmp = _fd;
    _fd = -1;
    return tmp;
}
