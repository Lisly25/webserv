#include <fcntl.h>
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <utility>

ScopedSocket::ScopedSocket(int fd, bool set_default_flags)
    : _fd(fd)
{
    if (set_default_flags && _fd != -1)
        setSocketFlags(FD_CLOEXEC | O_NONBLOCK);
}

ScopedSocket::~ScopedSocket()
{
    if (_fd != -1)
        close(_fd);
}

ScopedSocket::ScopedSocket(ScopedSocket&& other) noexcept
    : _fd(std::exchange(other._fd, -1))
{
}

ScopedSocket& ScopedSocket::operator=(ScopedSocket&& other) noexcept {
    if (this != &other)
    {
        reset();
        _fd = std::exchange(other._fd, -1);
    }
    return *this;
}

int ScopedSocket::get(void) const { return _fd; }

void ScopedSocket::reset(int fd)
{
    if (_fd != -1)
        close(_fd);
    _fd = fd;
}

int ScopedSocket::release(void)
{
    int fd = _fd;
    _fd = -1;
    return fd;
}

void ScopedSocket::setSocketFlags(int flags)
{
    int oldFlags = fcntl(_fd, F_GETFL, 0);
    if (oldFlags == -1 || fcntl(_fd, F_SETFL, oldFlags | flags) == -1)
        throw std::runtime_error("Failed to set socket flags");
}
