#pragma once

#include <cstdio>
#include <netdb.h>


class RequestHandler
{
public:
    RequestHandler();
    ~RequestHandler();


    void handleRequest(int clientSocket);
    
private:
    addrinfo*   _proxyInfo;

    bool    parseRequest(const char* request, ssize_t length);
    void    handleCgiRequest(const char* request, ssize_t length);
    void    handleProxyRequest(int clientSocket, const char* request, ssize_t length);
    void    resolveProxyAddress();
};
