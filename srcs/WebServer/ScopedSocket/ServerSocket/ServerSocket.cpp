#include "ServerSocket.hpp"
#include "WebParser.hpp"

ServerSocket::ServerSocket(int fd, const Server& server, int socket_flags)
    : ScopedSocket(fd, socket_flags), _server(server) {}

ServerSocket::ServerSocket(ServerSocket&& other) noexcept
    : ScopedSocket(std::move(other)), _server(other._server) {}

const Server& ServerSocket::getServer() const { return _server; }
