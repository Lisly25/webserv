#pragma once

#include "ScopedSocket.hpp"
#include "Request.hpp"
#include <string>

class ProxyHandler
{
public:
    ProxyHandler(const Request& request);
    void passRequest(std::string &response);

private:
    const Request& request;
    ScopedSocket createProxySocket(addrinfo* proxyInfo);
    void sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest);
    bool isDataAvailable(int fd, int timeout_usec);
    std::string modifyRequestForProxy();

    addrinfo* proxyInfo;
    std::string proxyHost;
};
