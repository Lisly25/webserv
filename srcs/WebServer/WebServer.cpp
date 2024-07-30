#include "WebServer.hpp"
#include "WebErrors.hpp"
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

bool WebServer::_running = true;

WebServer::WebServer(const std::string &proxyPass, int port)
    : _proxyPass(proxyPass)
{
    _serverSocket = createServerSocket(port);
    setSocketFlags(_serverSocket);

    _epollFd = epoll_create1(0);
    if (_epollFd == -1)
        throw WebErrors::ServerException("Error creating epoll instance");

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = _serverSocket;

    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverSocket, &event) == -1)
        throw WebErrors::ServerException("Error adding server socket to epoll");
}

void WebServer::setSocketFlags(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0)
        throw WebErrors::ServerException("fcntl(F_GETFL) failed");
    flags |= O_NONBLOCK | O_CLOEXEC;
    if (fcntl(socket, F_SETFL, flags) < 0)
        throw WebErrors::ServerException("fcntl(F_SETFL) failed");
}

WebServer::~WebServer()
{
    close(_serverSocket);
    close(_epollFd);
}

int WebServer::createServerSocket(int port)
{
    int opt = 1;
    const int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0 || 
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw WebErrors::ServerException("Error opening or configuring server socket");

    memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
        throw WebErrors::ServerException("Error binding server socket");

    if (listen(serverSocket, 1000) < 0)
        throw WebErrors::ServerException("Error listening on server socket");

    return serverSocket;
}

void WebServer::addClientSocket(int clientSocket)
{
    setSocketFlags(clientSocket);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = clientSocket;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        throw WebErrors::ServerException("Error adding client socket to epoll");
}

void WebServer::removeClientSocket(int clientSocket)
{
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
    close(clientSocket);
}

void WebServer::handleClient(int clientSocket)
{
    char        buffer[4096];
    addrinfo    hints{};
    addrinfo    *res = nullptr;
    ssize_t     bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0)
        throw WebErrors::ClientException("Error reading from client socket", nullptr, this, clientSocket);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("localhost", "8080", &hints, &res) != 0)
        throw WebErrors::ClientException("Error resolving proxy host", nullptr, this, clientSocket);

    const int proxySocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (proxySocket < 0 || connect(proxySocket, res->ai_addr, res->ai_addrlen) < 0)
        throw WebErrors::ClientException("Error connecting to proxy server", res, this, clientSocket);

    if (send(proxySocket, buffer, bytesRead, 0) < 0)
        throw WebErrors::ClientException("Error sending to proxy server", res, this, clientSocket);

    while ((bytesRead = recv(proxySocket, buffer, sizeof(buffer), 0)) > 0)
    {
        if (send(clientSocket, buffer, bytesRead, 0) < 0)
            throw WebErrors::ClientException("Error sending to client socket", res, this, clientSocket);
    }
    if (bytesRead < 0)
        throw WebErrors::ClientException("Error reading from proxy server", res, this, clientSocket);
    freeaddrinfo(res);
    close(proxySocket);
}

void WebServer::start()
{
    std::cout << "Server is running. Press Ctrl+C to stop.\n";
    signal(SIGINT, [](int signum) { (void)signum; WebServer::_running = false; });


    while (_running)
    {
        int eventCount = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (eventCount == -1) {
            if (errno == EINTR) continue;
            WebErrors::printerror("Epoll wait error");
            break;
        }

        for (int i = 0; i < eventCount; ++i) {
            if (_events[i].events & EPOLLIN) {
                if (_events[i].data.fd == _serverSocket) {
                    struct sockaddr_in clientAddr;
                    socklen_t clientLen = sizeof(clientAddr);
                    int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
                    if (clientSocket < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            WebErrors::printerror("Error accepting connection");
                        }
                        continue;
                    }
                    addClientSocket(clientSocket);
                } else {
                    try {
                        handleClient(_events[i].data.fd);
                    } catch (const WebErrors::ClientException &e) {
                        WebErrors::printerror(e.what());
                    }
                }
            }
        }
    }

    std::cout << "Server stopped.\n";
}
