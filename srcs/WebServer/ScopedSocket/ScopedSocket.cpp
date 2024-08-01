#include <fcntl.h>
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"

ScopedSocket::ScopedSocket(int fd) : _fd(fd) { setSocketFlags(O_NONBLOCK | O_CLOEXEC); }

ScopedSocket::~ScopedSocket() { if (_fd != -1) close(_fd);  }

int ScopedSocket::get() const { return _fd;  }

void ScopedSocket::reset(int fd)
{
    if (_fd != -1)
        close(_fd);
    _fd = fd;
    setSocketFlags(O_NONBLOCK | O_CLOEXEC);
}

int ScopedSocket::release()
{
    int tmp = _fd;
    _fd = -1;
    return tmp;
}

void ScopedSocket::setSocketFlags(int flags)
{
    int currentFlags = fcntl(_fd, F_GETFL, 0);
    if (currentFlags < 0)
        throw WebErrors::ServerException("fcntl(F_GETFL) failed: ");
    currentFlags |= flags;
    if (fcntl(_fd, F_SETFL, currentFlags) < 0)
        throw WebErrors::ServerException("fcntl(F_SETFL) failed: ");
}
