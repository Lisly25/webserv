#pragma once 


#include <string>

class Request
{
public:
    Request() = default;
    explicit Request(const std::string &rawRequest) : rawRequest(rawRequest) {
    }
    const std::string& getRawRequest() const { return rawRequest; }

private:
    std::string rawRequest;
};