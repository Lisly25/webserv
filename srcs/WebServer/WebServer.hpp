#pragma once

#include "WebParser.hpp"
#include <string>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <vector>

class WebServer {
public:
    WebServer(WebParser &parser, int port);
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void start();
    void removeClientSocket(int clientSocket);

private:
    static bool         _running;
    int                 _serverSocket;
    int                 _epollFd;
    WebParser           &_parser;
    struct sockaddr_in  _serverAddr;
    static const int    MAX_EVENTS = 100;
    epoll_event         _events[MAX_EVENTS];

    int     createServerSocket(int port);
    void    handleClient(int clientSocket);
    void    setSocketFlags(int socket);
    void    addClientSocket(int clientSocket);
    void    handleEvents(int eventCount);
    void    acceptClient(void);
    void    processClient(int clientSocket);
};