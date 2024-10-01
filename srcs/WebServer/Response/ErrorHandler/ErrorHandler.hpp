#pragma once
#include "Request.hpp"
#include <string>

class ErrorHandler
{
public:
    ErrorHandler(Request& request);
    void handleError(std::string& response, int errorCode) const;
    static std::string generateDefaultErrorPage(int errorCode);
private:
    Request& _request;

    std::string getErrorMessage(int errorCode) const;
    std::string getErrorPage(int errorCode) const;
    void        readFileContent(const std::string& path, std::string& content) const;
};
