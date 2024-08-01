#pragma once

#include "RequestHandler/RequestHandler.hpp"
#include "ScopedSocket.hpp"
#include "WebParser.hpp"
#include <string>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <vector>

class WebServer
{
public:
    WebServer(WebParser &parser, int port);
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void start();
    void removeClientSocket(int clientSocket);

private:
    static bool             _running;
    ScopedSocket            _serverSocket;
    int                     _epollFd = -1;
    WebParser               &_parser;
    struct sockaddr_in      _serverAddr;
    static const int        MAX_EVENTS = 100;
    epoll_event             _events[MAX_EVENTS];
    static RequestHandler   _requestHandler;

    int     createServerSocket(int port);
    void    handleClient(int clientSocket);
    void    setSocketFlags(int socket);
    void    addClientSocket(int clientSocket);
    void    handleEvents(int eventCount);
    void    acceptAddClient(void);

    void    handleOutgoingData(int clientSocket); // send()
    void    handleIncomingData(int clientSocket); // recv()

};