#include "ServerSocket.hpp"
#include "WebErrors.hpp"
#include "WebParser.hpp"

ServerSocket::ServerSocket(const Server& server, int socket_flags)
    : ScopedSocket(socket(AF_INET, SOCK_STREAM, 0), socket_flags), _server(server)
{
    try
    {
        if (this->getFd() < 0) 
            throw WebErrors::ServerException("Error opening server socket for server on port " + std::to_string(server.port));
        
        setupSocketOptions(1);
        bindAndListen();
    }
    catch (const std::exception &e)
    {
        throw ;
    }
}

ServerSocket::ServerSocket(ServerSocket&& other) noexcept
    : ScopedSocket(std::move(other)), _server(other._server)
{
}

const Server& ServerSocket::getServer() const { return _server; }

void ServerSocket::setupSocketOptions(int opt)
{
    if (setsockopt(getFd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw WebErrors::ServerException("Error setting socket options for server on port " + std::to_string(_server.port));
}

void ServerSocket::bindAndListen()
{
    std::memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(_server.port);

    if (bind(getFd(), (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
        throw WebErrors::ServerException("Error binding server socket on port " + std::to_string(_server.port));

    if (listen(getFd(), SOMAXCONN) < 0)
        throw WebErrors::ServerException("Error listening on server socket on port " + std::to_string(_server.port));
}
