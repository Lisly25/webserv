#pragma once

#include "ScopedSocket.hpp"
#include "ServerSocket.hpp"
#include "WebParser.hpp"
#include <netdb.h>
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
    WebServer(WebParser &parser);
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void start();
    void epollController(int clientSocket, int operation, uint32_t events);

private:
    std::vector<ServerSocket>                   _serverSockets = {};
    static bool                                 _running;
    int                                         _epollFd = -1;
    WebParser                                   &_parser;
    std::vector<struct epoll_event>             _events = {};

    std::unordered_map<std::string, addrinfo*>  _proxyInfoMap = {};

    std::unordered_map<int, Request> _requestMap;

    std::vector<ServerSocket>   createServerSockets(const std::vector<Server> &server_confs);
    void                        handleClient(int clientSocket);
    void                        setSocketFlags(int socket);
    void                        addClientSocket(int clientSocket);
    void                        handleEvents(int eventCount);
    void                        acceptAddClientToEpoll(int serverSocketFd);

    void                        handleOutgoingData(int clientSocket); // send()
    void                        handleIncomingData(int clientSocket); // recv()

    void                        resolveProxyAddresses(const std::vector<Server>& server_confs);

    int                         getRequestTotalLength(const std::string &request);
    std::string                 getBoundary(const std::string &request);
};
