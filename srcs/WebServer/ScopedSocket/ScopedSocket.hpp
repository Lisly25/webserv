#pragma once 

#include <unistd.h>

class ScopedSocket {
public:
    ScopedSocket(int fd = -1, bool set_default_flags = true);
    ~ScopedSocket();

    ScopedSocket(const ScopedSocket&) = delete;            // Disable copy constructor
    ScopedSocket& operator=(const ScopedSocket&) = delete; // Disable copy assignment

    ScopedSocket(ScopedSocket&& other) noexcept;            // Declare move constructor
    ScopedSocket& operator=(ScopedSocket&& other) noexcept; // Declare move assignment operator

    int     get(void) const;
    void    reset(int fd = -1);
    int     release(void);

private:
    int     _fd;

    void    setSocketFlags(int flags);
};
