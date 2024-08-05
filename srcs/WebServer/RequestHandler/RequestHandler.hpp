#pragma once

#include "ScopedSocket.hpp"
#include <cstdio>
#include <netdb.h>
#include <unordered_map>
#include <string>

class RequestHandler
{
public:
    RequestHandler(void);
    ~RequestHandler();

    void        handleRequest(int clientSocket);
    void        handleProxyResponse(int proxySocket);
    void        storeRequest(int clientSocket, const std::string &request);
    std::string generateResponse(int clientSocket);

private:
    addrinfo* _proxyInfo = nullptr;
    int proxySocket = -1;
    std::unordered_map<int, std::string> _requestMap;

    bool parseRequest(const char* request, ssize_t length);
    void handleCgiPass(const char* request, ssize_t length);
    void handleProxyPass(const std::string &request, std::string &response);

    void resolveProxyAddresses();
    void connectToProxy(void);
};
