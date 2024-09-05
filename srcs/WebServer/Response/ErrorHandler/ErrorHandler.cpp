#include "ErrorHandler.hpp"

ErrorHandler::ErrorHandler(const Request& request)
    : _request(request)
{
}

void ErrorHandler::handleError(std::string& response) const
{
    int errorCode = _request.getErrorCode();
    std::string errorMessage = getErrorMessage(errorCode);
    std::string errorPagePath = WebParser::getErrorPage(errorCode, _request.getServer());
    std::string errorPage;
    try
    {
        readFileContent(errorPagePath, errorPage);
    }
    catch(const std::exception& e)
    {
        response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        return;
    }
    

    response = "HTTP/1.1 " + std::to_string(errorCode) + " " + errorMessage + "\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Content-Length: " + std::to_string(errorPage.length()) + "\r\n";
    response += "\r\n";
    response += errorPage;
}

std::string ErrorHandler::getErrorMessage(int errorCode) const
{
    switch (errorCode)
    {
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 505: return "HTTP Version Not Supported";
    default: return "Internal Server Error";
    }
}

void ErrorHandler::readFileContent(const std::string& path, std::string& content) const
{
    std::ifstream fileStream(path, std::ios::in | std::ios::binary);
    if (!fileStream)
    {
        throw std::runtime_error("Failed to read file: " + path);
    }

    std::ostringstream ss;
    ss << fileStream.rdbuf();
    content = ss.str();

}