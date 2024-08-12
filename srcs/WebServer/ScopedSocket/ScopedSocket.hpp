#pragma once 

#include <unistd.h>

class ScopedSocket {
public:
    ScopedSocket(int fd = -1, bool set_default_flags = true);
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
