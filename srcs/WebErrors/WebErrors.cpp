#include "WebErrors.hpp"
#include <cstring>
#include "WebServer.hpp"

namespace WebErrors
{
    BaseException::BaseException(const std::string &message) : _message(message) {}

    const char* BaseException::what() const noexcept
    {
        return _message.c_str();
    }

    FileOpenException::FileOpenException(const std::string &filename)
        : BaseException("Error opening config file: " + filename) {}

    /* Handles exceptions happening in the client handling in server */
    ClientException::ClientException(const std::string &message, addrinfo* res, WebServer* server, int clientSocket)
        : BaseException(message), _res(res), _server(server), _clientSocket(clientSocket) { }

    ClientException::~ClientException()
    {
        if (_res) freeaddrinfo(_res);
        if (_server && _clientSocket != -1) _server->removeClientSocket(_clientSocket);
    }
    
    
    int printerror(const std::string &e)
    {
        if (errno != 0)
            std::cerr << e << ": " << strerror(errno) << std::endl;
        else
            std::cerr << e << std::endl;
        return -1;
    }

    /* Server Exceptions in the server */
    ServerException::ServerException(const std::string &message)
        : BaseException(message) { }
    ServerException::~ServerException() {}
}
