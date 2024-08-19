#pragma once 

#include "ScopedSocket.hpp"
#include "WebParser.hpp"
#include <netinet/in.h>

class ServerSocket : public ScopedSocket
{
public:
    ServerSocket(const Server& server, int socket_flags = 0);
    ServerSocket(ServerSocket&& other) noexcept;

    ServerSocket& operator=(ServerSocket&& other) noexcept = delete;

    const Server& getServer() const;

private:
    void setupSocketOptions(int opt);
    void bindAndListen();

    const Server&       _server;
    struct sockaddr_in  _serverAddr = {};
};
