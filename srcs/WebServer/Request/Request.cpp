#include "Request.hpp"

Request::Request(const std::string &rawRequest) : rawRequest(rawRequest) {}

Request::Request(const Request &other) : rawRequest(other.rawRequest) {}

Request& Request::operator=(const Request &other)
{
    if (this != &other)
        rawRequest = other.rawRequest;
    return *this;
}
