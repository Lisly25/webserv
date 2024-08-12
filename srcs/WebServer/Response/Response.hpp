#pragma once

#include <string>
#include <netdb.h>

class Request;

class Response
{
public:
    Response(addrinfo* proxyInfo = nullptr);
    ~Response();

    std::string generate(const Request &request);

private:
    addrinfo* _proxyInfo;

    void handleProxyPass(const std::string &request, std::string &response);
};
