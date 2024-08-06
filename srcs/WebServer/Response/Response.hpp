#pragma once

#include "Request.hpp"
#include <string>
#include <netdb.h>

class Response {
public:
    Response();
    ~Response();
    std::string generate(const Request &request);
    void handleProxyPass(const std::string &request, std::string &response);

private:
    void resolveProxyAddresses();
    addrinfo *_proxyInfo;
};