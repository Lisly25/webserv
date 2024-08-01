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
    void    handleCgiPass(const char* request, ssize_t length);
    void    handleProxyPass(int clientSocket, const char* request, ssize_t length);
    void    resolveProxyAddress();
};
