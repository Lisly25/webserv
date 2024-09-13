#include <fcntl.h>
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <utility>

ScopedSocket::ScopedSocket(int fd, int socket_flags)
    : _fd(fd),  _ownsSocket(true) 
{
    if (_fd != -1)
    {
        try {
            setSocketFlags(socket_flags);
        } catch (const std::exception &e) {
            throw;
        }
    }
}

ScopedSocket::~ScopedSocket()
{
    if (_fd != -1 && _ownsSocket)
    {
        close(_fd);
    }
}

ScopedSocket::ScopedSocket(ScopedSocket&& other) noexcept
    : _fd(std::exchange(other._fd, -1))
{
}

ScopedSocket& ScopedSocket::operator=(ScopedSocket&& other) noexcept
{
    if (this != &other)
    {
        reset();
        _fd = std::exchange(other._fd, -1);
    }
    return *this;
}

int ScopedSocket::getFd(void) const { return _fd; }

void ScopedSocket::reset(int fd)
{
    if (_fd != -1)
        close(_fd);
    _fd = fd;
}

int ScopedSocket::release(bool closeSocket)
{
    if (!closeSocket)
        _ownsSocket = false;
    int fd = _fd;
    _fd = -1;
    return fd;
}

void ScopedSocket::setSocketFlags(int flags)
{
    if (_fd == -1)
        throw std::runtime_error("Invalid socket file descriptor");

    int oldFlags = fcntl(_fd, F_GETFL, 0);
    if (oldFlags == -1 || fcntl(_fd, F_SETFL, oldFlags | flags) == -1)
        throw WebErrors::SocketException("Failed to set socket flags");
}
