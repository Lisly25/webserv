#pragma once

#include "ScopedSocket.hpp"
#include <string>
#include <netdb.h>

class Request;

class Response
{
public:
    void            handleProxyPass(const Request& request, std::string &response);
    std::string     generate(const Request &request);
private:
    ScopedSocket    createProxySocket(addrinfo* proxyInfo);
    void            sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest);
    void            receiveResponseFromProxy(ScopedSocket& proxySocket, std::string &response, const std::string& proxyHost);

    bool            isDataAvailable(int fd, int timeout_usec);
};
