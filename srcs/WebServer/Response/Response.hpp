#pragma once

#include "ScopedSocket.hpp"
#include <string>
#include <netdb.h>

class Request;

class Response
{
public:
    Response(Request &request);
    ~Response() = default;

    const std::string   &getResponse() const;

private:
    std::string    _response;

    ScopedSocket    createProxySocket(addrinfo* proxyInfo);
    void            sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest);
    void            receiveResponseFromProxy(ScopedSocket& proxySocket, std::string &response, const std::string& proxyHost);
    bool            isDataAvailable(int fd, int timeout_usec);
    std::string     generate(Request &request);
};
