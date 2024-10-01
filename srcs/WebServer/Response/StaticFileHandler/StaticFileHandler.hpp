#pragma once
#include "Request.hpp"
#include "Response.hpp"

class StaticFileHandler
{
public:
    StaticFileHandler(Request& request);
    void serveFile(std::string& response);

private:
    Request& _request;

    void        handleCookies(Request &request, std::string &response);
    bool        fileExists(const std::string& path) const;
    std::string getMimeType(const std::string& path) const;
    void        readFileContent(const std::string& path, std::string& content) const;
};
