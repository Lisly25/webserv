#pragma once
#include "Request.hpp"
#include "Response.hpp"

class StaticFileHandler
{
public:
    StaticFileHandler(const Request& request);
    void serveFile(std::string& response);

private:
    const Request& _request;

    bool fileExists(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    void readFileContent(const std::string& path, std::string& content) const;
};
