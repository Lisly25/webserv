#pragma once 

#include "ScopedSocket.hpp"
#include "WebParser.hpp"

class ServerSocket : public ScopedSocket
{
public:
    ServerSocket(int fd, const Server& server, int socket_flags = 0);

    ServerSocket(ServerSocket&& other) noexcept;

    ServerSocket& operator=(ServerSocket&& other) noexcept = delete;

    const Server& getServer() const;

private:
    const Server& _server;
};
