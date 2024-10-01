#pragma once

#include "ScopedSocket.hpp"
#include "ServerSocket.hpp"
#include "WebParser.hpp"
#include <csignal>
#include <list>
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

struct CGIProcessInfo
{
    int         readFromCgiFd;
    int         writeToCgiFd;
    pid_t       pid;
    int         clientSocket;
    std::string response;
    std::chrono::steady_clock::time_point startTime;
};

struct ClientInfo
{
    int           clientSocket;
    const Server  *server_conf;
    std::chrono::steady_clock::time_point   lastActivity;
};

using cgiInfoList = std::list<CGIProcessInfo>;
using clientInfoMap = std::unordered_map<int, ClientInfo>;
using proxyInfoMap = std::unordered_map<std::string, addrinfo*>;
using requestMap = std::unordered_map<int, Request>;
using partialRequestMap = std::unordered_map<int, std::string>;

enum FdType  {SERVER, CLIENT, CGI_PIPE };

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
    cgiInfoList          &getCgiInfoList();
    int                  getCurrentEventFd() const;

    static void          setFdNonBlocking(int fd);
private:
    static volatile sig_atomic_t                s_serverRunning;
    std::vector<ServerSocket>                   _serverSockets = {};
    int                                         _epollFd = -1;
    int                                         _currentEventFd = -1;
    WebParser                                   &_parser;
    std::vector<struct epoll_event>             _events = {};

    partialRequestMap                           _partialRequests;
    cgiInfoList                                 _cgiInfoList = {};
    proxyInfoMap                                _proxyInfoMap = {};
    requestMap                                  _requestMap = {};
    clientInfoMap                               _clientInfoMap = {};

    std::vector<ServerSocket>   createServerSockets(const std::vector<Server> &server_confs);
    void                        handleClient(int clientSocket);
    void                        handleEvents(int eventCount);
    void                        acceptAddClientToEpoll(int serverSocketFd);
    void                        resolveProxyAddresses(const std::vector<Server>& server_confs);

    void                        handleCGIinteraction(int pipeFd); // read() && send() for CGI
    void                        handleIncomingData(int clientSocket); // recv()
    void                        handleOutgoingData(int clientSocket); // send()
    void                        timeoutChecker(void);
    void                        cleanupClient(int clientSocket);
    void                        processRequest(int clientSocket, const std::string &requestStr);
    bool                        isRequestComplete(const std::string &request);
    std::string                 extractCompleteRequest(const std::string &buffer);

    static void                 signalHandler(int signal);
};
