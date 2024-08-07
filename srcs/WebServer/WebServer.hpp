#pragma once

#include "ScopedSocket.hpp"
#include "WebParser.hpp"
#include <string>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <unordered_map>
#include <vector>
#include "Request.hpp"

#define MAX_EVENTS 100

class WebServer
{
public:
    WebServer(WebParser &parser, int port);
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void start();
    void epollController(int clientSocket, int operation, uint32_t events);

private:
    static bool             _running;
    ScopedSocket            _serverSocket;
    int                     _epollFd = -1;
    WebParser               &_parser;
    struct sockaddr_in      _serverAddr;

    std::vector<struct epoll_event> _events;

    std::unordered_map<int, Request> _requestMap;

    int     createServerSocket(int port);
    void    handleClient(int clientSocket);
    void    setSocketFlags(int socket);
    void    addClientSocket(int clientSocket);
    void    handleEvents(int eventCount);
    void    acceptAddClient(void);

    void    handleOutgoingData(int clientSocket); // send()
    void    handleIncomingData(int clientSocket); // recv()



    int         getRequestTotalLength(const std::string &request);
    std::string getBoundary(const std::string &request);

};