#pragma once

#include <string>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <vector>

class WebServer {
public:
    WebServer(const std::string &proxyPass, int port);
    ~WebServer();

    void start();
    void removeClientSocket(int clientSocket);

private:
    static bool         _running;
    int                 _serverSocket;
    int                 _epollFd;
    std::string         _proxyPass;
    struct sockaddr_in  _serverAddr;
    static const int    MAX_EVENTS = 1024;
    epoll_event         _events[MAX_EVENTS];

    int     createServerSocket(int port);
    void    handleClient(int clientSocket);
    void    setSocketFlags(int socket);
    void    addClientSocket(int clientSocket);
};