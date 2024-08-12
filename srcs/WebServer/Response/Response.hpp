#pragma once

#include <string>
#include <netdb.h>

class Request;

class Response
{
public:
    Response() = default;
    ~Response() = default;

    std::string generate(const Request &request);

private:

    void handleProxyPass(const Request& request, std::string &response);
};
