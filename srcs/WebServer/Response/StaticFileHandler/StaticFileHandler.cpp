#include "StaticFileHandler.hpp"
#include <filesystem>
#include <numeric>

StaticFileHandler::StaticFileHandler(const Request& request) 
    : _request(request) {}




void StaticFileHandler::serveFile(std::string& response)
{
    const std::string& fullPath = _request.getRequestData().uri;

    if (std::filesystem::exists(fullPath) && fullPath == "/fun_facts/index.html") {
        handleCookies(_request, response);
        response += "HTTP/1.1 302 Found\r\n";
        response += "Location: /home.html\r\n";
        response += "Cache-Control: max-age=3600\r\n";
        response += "\r\n";
        return;
    }

    if (!std::filesystem::exists(fullPath)) {
        response += "HTTP/1.1 404 Not Found\r\n\r\n";
        return;
    }

    std::string fileContent;
    try {
        readFileContent(fullPath, fileContent);
    } catch (const std::exception& e) {
        response += "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        return;
    }

    std::string mimeType = getMimeType(fullPath);
    response += "HTTP/1.1 200 OK\r\n";
    handleCookies(_request, response);
    response += "Content-Type: " + mimeType + "\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "Cache-Control: max-age=3600\r\n";
    response += "\r\n" + fileContent;
}

void StaticFileHandler::handleCookies(const Request &request, std::string &response)
{
    auto cookies = request.getRequestData().cookies;

    if (cookies.find("visit_status") == cookies.end())
    {
        std::cout << "First-time visitor. Setting visit_status to 'first_visit'.\n";

        auto expiryTime = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now() + std::chrono::hours(5)
        );

        response += "Set-Cookie: visit_status=first_visit; Path=/; Max-Age=18000\r\n";
        response += "Set-Cookie: visit_expiry=" + std::to_string(expiryTime * 1000) + "; Path=/; Max-Age=18000\r\n";
    } 
    else if (cookies["visit_status"] == "first_visit")
    {
        std::cout << "Returning visitor. Updating to 'return_visit'.\n";
        response += "Set-Cookie: visit_status=return_visit; Path=/; Max-Age=18000\r\n";
    } 
    else
        std::cout << "Returning visitor with 'return_visit'.\n";
}



std::string StaticFileHandler::getMimeType(const std::string& path) const
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

void StaticFileHandler::readFileContent(const std::string& path, std::string& content) const
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
