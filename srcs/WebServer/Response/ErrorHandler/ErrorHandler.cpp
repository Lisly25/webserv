#include "ErrorHandler.hpp"

ErrorHandler::ErrorHandler(const Request& request)
    : _request(request)
{
}

void ErrorHandler::handleError(std::string& response) const
{
    int errorCode = _request.getErrorCode();
    std::string errorMessage = getErrorMessage(errorCode);
    std::string errorPage = getErrorPage(errorCode);

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

std::string ErrorHandler::getErrorPage(int errorCode) const
{
    // Generate a simple HTML error page
    std::string errorPage = "<html><head><title>" + getErrorMessage(errorCode) + "</title></head><body>";
    errorPage += "<h1>" + getErrorMessage(errorCode) + "</h1>";
    errorPage += "<p>The server encountered an error and could not complete your request.</p>";
    errorPage += "</body></html>";

    return errorPage;
}
