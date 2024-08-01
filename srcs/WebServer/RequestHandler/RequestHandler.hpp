#pragma once

#include <cstdio>
#include <netdb.h>


class RequestHandler
{
public:
    RequestHandler(int epollFd);
    ~RequestHandler();


    void handleRequest(int clientSocket);
    void handleProxyResponse(int proxySocket);
    
private:
    addrinfo*   _proxyInfo;
    int         _proxySocket = -1;
    int         _epollFd = -1;

    bool    parseRequest(const char* request, ssize_t length);
    void    handleCgiPass(const char* request, ssize_t length);
    void    handleProxyPass(int clientSocket, const char* request, ssize_t length);
    void    resolveProxyAddress();
    void    connectToProxy(void);
};
