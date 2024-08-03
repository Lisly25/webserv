#include "WebServer.hpp"
#include "RequestHandler/RequestHandler.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <csignal>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

bool WebServer::_running = true;

WebServer::WebServer(WebParser &parser, int port)
    : _serverSocket(createServerSocket(port)), _epollFd(epoll_create(1)), _parser(parser), _requestHandler(_epollFd)
{
    if (_epollFd == -1)
        throw WebErrors::ServerException("Error creating epoll instance");

    epoll_event event;
    std::memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    event.data.fd = _serverSocket.get();
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, _serverSocket.get(), &event) == -1)
        throw WebErrors::ServerException("Error adding server socket to epoll");
}


WebServer::~WebServer() { close(_epollFd); }

int WebServer::createServerSocket(int port)
{
    int             opt = 1;
    ScopedSocket    serverSocket(socket(AF_INET, SOCK_STREAM, 0));

    if (serverSocket.get() < 0 || 
        setsockopt(serverSocket.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw WebErrors::ServerException("Error opening or configuring server socket");

    std::memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(port);
    if (bind(serverSocket.get(), (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
        throw WebErrors::ServerException("Error binding server socket");

    if (listen(serverSocket.get(), 1000) < 0)
        throw WebErrors::ServerException("Error listening on server socket");

    return serverSocket.release();
}

// ACCEPT THE CLIENT CONNECTION AND ADD IT TO THE EPOLL POOL
void WebServer::acceptAddClient()
{
    struct sockaddr_in  clientAddr;
    epoll_event         event;
    socklen_t           clientLen = sizeof(clientAddr);
    ScopedSocket        clientSocket(accept(_serverSocket.get(), (struct sockaddr *)&clientAddr, &clientLen));

    if (clientSocket.get() < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw WebErrors::ServerException("Error accepting client connection");

    event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
    event.data.fd = clientSocket.get();
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientSocket.get(), &event) == -1)
        throw WebErrors::ServerException("Error adding client socket to epoll");

    clientSocket.release();
}

void WebServer::removeClientSocket(int clientSocket)
{
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
    close(clientSocket);
    clientSocket = -1;
}

void WebServer::handleIncomingData(int clientSocket)
{
    try {
        if (clientSocket == _serverSocket.get())
            acceptAddClient();
        else
            _requestHandler.handleRequest(clientSocket);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void WebServer::handleOutgoingData(int socket)
{
    try {
        _requestHandler.handleProxyResponse(socket);
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void WebServer::handleEvents(int eventCount) {
    for (int i = 0; i < eventCount; ++i) {
        if (_events[i].events & EPOLLIN)
            handleIncomingData(_events[i].data.fd);
        else if (_events[i].events & EPOLLOUT)
            handleOutgoingData(_events[i].data.fd);
    }
}


void WebServer::start()
{
    std::cout << "Server is running. Press Ctrl+C to stop.\n";
    std::signal(SIGINT, [](int signum) { (void)signum; WebServer::_running = false; });

    while (_running)
    {
        int eventCount = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (eventCount == -1)
        {
            if (errno == EINTR) continue;
            WebErrors::printerror("Epoll wait error");
            continue;
        }
        handleEvents(eventCount);
    }
    std::cout << "Server stopped.\n";
}