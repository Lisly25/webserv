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

#define COLOR_RED_ERROR "\033[31m"
#define COLOR_CYAN_COOKIE "\033[36m"
#define COLOR_GREEN_SERVER "\033[32m"
#define COLOR_MAGENTA_SERVER "\033[35m"
#define COLOR_YELLOW_CGI "\033[33m"
#define COLOR_RESET "\033[0m"

class  ProxyHandler;

struct CGIProcessInfo
{
    pid_t       pid;
    int         clientSocket;
    std::string response;
};
using cgiInfoMap = std::unordered_map<int, CGIProcessInfo>;
using proxyHandlerMap = std::unordered_map<int, std::unique_ptr<ProxyHandler>> ;

enum FdType  {SERVER_FD, CLIENT_FD, CGI_PIPE_FD, PROXY_FD };

class WebServer
{
public:
    WebServer(WebParser &parser);
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void                 start();
    void                 epollController(int clientSocket, int operation, uint32_t events, FdType fdType);
    int                  getEpollFd() const;
    cgiInfoMap           &getCgiInfoMap();
    int                  getCurrentEventFd() const;
    proxyHandlerMap      &getProxyHandlerMap(); 
    static void          setFdNonBlocking(int fd);
private:
    std::vector<ServerSocket>                   _serverSockets = {};
    static bool                                 _running;
    int                                         _epollFd = -1;
    int                                         _currentEventFd = -1;
    WebParser                                   &_parser;
    std::vector<struct epoll_event>             _events = {};

    std::unordered_map<int, std::string>        _partialRequests;
    cgiInfoMap                                  _cgiInfoMap;
    proxyHandlerMap                             _proxyHandlers;
    std::unordered_map<std::string, addrinfo*>  _proxyAddrInfoMap = {};
    std::unordered_map<int, Request>            _requestMap;

    std::vector<ServerSocket>   createServerSockets(const std::vector<Server> &server_confs);
    void                        handleClient(int clientSocket);
    void                        handleEvents(int eventCount);
    void                        acceptAddClientToEpoll(int serverSocketFd);
    void                        resolveProxyAddresses(const std::vector<Server>& server_confs);

    void                        handleCGIinteraction(int pipeFd); // read() && send() for CGI
    void                        handleIncomingData(int clientSocket); // recv()
    void                        handleOutgoingData(int clientSocket); // send()
    void                        cleanupClient(int clientSocket);
    void                        processRequest(int clientSocket, const std::string &requestStr);
    bool                        isRequestComplete(const std::string &request);
    std::string                 extractCompleteRequest(const std::string &buffer);



};
