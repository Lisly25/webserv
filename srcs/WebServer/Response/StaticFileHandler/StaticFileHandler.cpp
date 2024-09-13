#include "StaticFileHandler.hpp"
#include <filesystem>
#include <numeric>
#include "ErrorHandler.hpp"
#include "WebErrors.hpp"
#include "WebServer.hpp"

StaticFileHandler::StaticFileHandler(const Request& request) 
    : _request(request) {}

void StaticFileHandler::serveFile(std::string& response)
{
    try
    {
        const std::string& fullPath = _request.getRequestData().uri;
        const bool isAutoIndex = std::filesystem::is_directory(fullPath) && _request.getLocation()->autoIndexOn;

        auto appendHeaders = [&](const std::string& status, const std::string& mimeType, size_t contentLength) {
            response += "HTTP/1.1 " + status + "\r\n";
            response += "Content-Type: " + mimeType + "\r\n";
            response += "Content-Length: " + std::to_string(contentLength) + "\r\n";
            response += "Cache-Control: max-age=3600\r\n";
        };

        if (isAutoIndex)
        {
            std::vector<std::string> indexPage = WebParser::generateIndexPage(fullPath);
            std::string content = std::accumulate(indexPage.begin(), indexPage.end(), std::string(""));

            appendHeaders("200 OK", "text/html", content.size());
            response += "\r\n" + content;
            return;
        }

        if (!std::filesystem::exists(fullPath))
        {
            ErrorHandler    errorHandler(_request);
            errorHandler.handleError(response, NOT_FOUND);
            return;
        }

        std::string fileContent;
        try {
            readFileContent(fullPath, fileContent);
        } catch (const std::exception& e) {
            ErrorHandler    errorHandlerServer(_request);
            errorHandlerServer.handleError(response, SERVER_ERROR);
            return;
        }

        std::string mimeType = getMimeType(fullPath);
        appendHeaders("200 OK", mimeType, fileContent.size());
        handleCookies(_request, response);
        response += "\r\n" + fileContent;
    }
    catch (const std::exception& e)
    {
        WebErrors::printerror("StaticFileHandler::serveFile", e.what());
        throw;
    }
}

void StaticFileHandler::handleCookies(const Request &request, std::string &response)
{
    auto addCookie = [&](const std::string& name, const std::string& value, int maxAge) {
        response += "Set-Cookie: " + name + "=" + value + "; Path=/; Max-Age=" + std::to_string(maxAge) + "\r\n";
    };

    auto handleFirstTime = [&]() {
        std::cout << COLOR_CYAN_COOKIE << "  Cookie: Update Age and visit status [ first_visit ] 🍪\n\n" << COLOR_RESET;
        auto expiryTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::hours(5));
        addCookie("visit_status", "first_visit", 18000);
        addCookie("visit_expiry", std::to_string(expiryTime * 1000), 18000);
    };

    auto setReturning = [&]() {
        std::cout << COLOR_CYAN_COOKIE << "  Cookie: Update Age and visit status [ return_visit ] 🍪\n\n" << COLOR_RESET;
        addCookie("visit_status", "return_visit", 18000);
    };

    try {

        auto cookies = request.getRequestData().cookies;

        if (cookies.find("visit_status") == cookies.end())
        {
            handleFirstTime();
        } 
        else if (cookies["visit_status"] == "first_visit")
        {
            setReturning();
        } 
        else
        {
            std::cout << COLOR_CYAN_COOKIE << "  Cookie: Update Age and visit status [ return_visit ] 🍪\n\n" << COLOR_RESET;
        }
    }
    catch (const std::exception& e)
    {
        WebErrors::printerror("StaticFileHandler::handleCookies", e.what());
        throw ;
    }

}

std::string StaticFileHandler::getMimeType(const std::string& path) const
{
    try 
    {
        if (path.find(".html") != std::string::npos)
            return "text/html";
        if (path.find(".css") != std::string::npos)
            return "text/css";
        if (path.find(".js") != std::string::npos)
            return "application/javascript";
        if (path.find(".png") != std::string::npos)
            return "image/png";
        if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
            return "image/jpeg";
        if (path.find(".gif") != std::string::npos)
            return "image/gif";
        return "application/octet-stream";
    }
    catch (const std::exception& e)
    {
        WebErrors::printerror("StaticFileHandler::getMimeType", e.what());
        throw;
    }
}

void StaticFileHandler::readFileContent(const std::string& path, std::string& content) const
{
    try
    {
        std::ifstream fileStream(path, std::ios::in | std::ios::binary);
        if (!fileStream)
        {
            throw std::runtime_error(  "Error: {StaticFileHandler::readFileContent}: " );
        }

        std::ostringstream ss;
        ss << fileStream.rdbuf();
        content = ss.str();
    }
    catch (const std::exception& e)
    {
        WebErrors::printerror("StaticFileHandler::readFileContent", e.what());
        throw;
    }
}
