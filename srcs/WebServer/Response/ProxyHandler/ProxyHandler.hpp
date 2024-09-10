#pragma once

#include "ScopedSocket.hpp"
#include "Request.hpp"
#include <string>

class ProxyHandler
{
public:
    ProxyHandler(const Request& request);
    ~ProxyHandler() = default;
    void passRequest(std::string &response);

private:
    const Request&  _request;
    addrinfo*       _proxyInfo;
    std::string     _proxyHost;
    void            sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest);
    bool            isDataAvailable(int fd, int timeout_usec);
    std::string     modifyRequestForProxy();
};
