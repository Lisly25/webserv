#pragma once 


#include <string>

class Request
{
public:
    Request() = default;
    Request(const std::string &rawRequest);
    Request(const Request &other);
    Request& operator=(const Request &other);
    ~Request() = default;

    const std::string& getRawRequest() const { return rawRequest; }

private:
    std::string rawRequest;
};
