#include "RequestHandler.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <algorithm>
#include <unistd.h>
#include <sys/epoll.h>

RequestHandler::RequestHandler(int epollFd) : _proxyInfo(nullptr), _epollFd(epollFd)
{
    resolveProxyAddress();
}

RequestHandler::~RequestHandler()
{
    freeaddrinfo(_proxyInfo);
    _proxyInfo = nullptr;
}

void RequestHandler::handleRequest(int clientSocket)
{
    char buffer[4096];
    const ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0)
        throw WebErrors::ClientException("Error reading from client socket", nullptr, nullptr, clientSocket);

    try {
        // ParseRequest()     parses and checks where we need to pass the data cgi or proxy
        handleProxyPass(clientSocket, buffer, bytesRead);
    } catch (const WebErrors::ClientException &e) {
        throw;
    }
}

void RequestHandler::handleProxyPass(int clientSocket, const char* request, ssize_t length)
{
    ScopedSocket proxySocket(socket(_proxyInfo->ai_family, _proxyInfo->ai_socktype, _proxyInfo->ai_protocol));

    if (proxySocket.get() < 0 || connect(proxySocket.get(), _proxyInfo->ai_addr, _proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ClientException("Error connecting to proxy server", _proxyInfo, nullptr, clientSocket);

    epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLOUT;
    event.data.fd = proxySocket.get();
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, proxySocket.get(), &event) == -1)
        throw WebErrors::ClientException("Error adding proxy socket to epoll", _proxyInfo, nullptr, clientSocket);

    _clientProxyMap[clientSocket] = proxySocket.get();

    if (send(proxySocket.get(), request, length, 0) < 0)
        throw WebErrors::ClientException("Error sending to proxy server", _proxyInfo, nullptr, clientSocket);
}

void RequestHandler::handleProxyResponse(int proxySocket)
{
    char    buffer[4096];
    ssize_t bytesRead = 0;
    int     clientSocket  = -1;

    auto it = _clientProxyMap.begin();
    for (; it != _clientProxyMap.end(); ++it)
    {
        if (it->second == proxySocket) break;
    }
    if (it == _clientProxyMap.end()) return;

    clientSocket = it->first;

    while ((bytesRead = recv(proxySocket, buffer, sizeof(buffer), 0)) > 0)
    {
        if (send(clientSocket, buffer, bytesRead, 0) < 0)
            throw WebErrors::ClientException("Error sending to client socket", _proxyInfo, nullptr, clientSocket);
    }

    if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw WebErrors::ClientException("Error reading from proxy server", _proxyInfo, nullptr, clientSocket);

    if (bytesRead == 0)
    {
        epoll_ctl(_epollFd, EPOLL_CTL_DEL, proxySocket, nullptr);
        close(proxySocket);
        _clientProxyMap.erase(it);
    }
}


// Find the locations from the config file
bool RequestHandler::parseRequest(const char* request, ssize_t length)
{

/**  CHECK THAT FROM WAHT LOCATION USER WANTS THE DATA AND DOES THE 
    *   DO WE PASS THE DATA TO CGI OR PROXY TO OTHER SERVER
    location /perse
    {
        proxy_pass http://localhost:8080;
        cgi_pass /usr/bin/php-cgi;
    }
 */

    (void )request;
    (void )length;
    return false;
}

void RequestHandler::handleCgiPass(const char* request, ssize_t length)
{
   (void )request;
    (void )length;
}

void RequestHandler::resolveProxyAddress()
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP or UDP
    hints.ai_flags = AI_PASSIVE;    // Allows bind

    // Only one proxy running on 8080, (homer docker container)
    if (getaddrinfo("localhost", "8080", &hints, &_proxyInfo) != 0)
        throw WebErrors::ClientException("Error resolving proxy host", nullptr, nullptr, -1);
}
