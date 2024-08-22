#include "StaticFileHandler.hpp"
#include <filesystem>
#include <numeric>

StaticFileHandler::StaticFileHandler(const Request& request) 
    : _request(request) {}

void StaticFileHandler::serveFile(std::string& response)
{
    const std::string& fullPath = _request.getRequestData().uri;
    std::cout << "Serving file at path: " << fullPath << std::endl;

    if (std::filesystem::is_directory(fullPath) && _request.getLocation()->autoIndexOn)
    {
        std::vector<std::string> indexPage = WebParser::generateIndexPage(fullPath);
        std::string content = std::accumulate(indexPage.begin(), indexPage.end(), std::string(""));

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
        response += "Cache-Control: max-age=3600\r\n";
        response += "\r\n" + content;
        return;
    }

    if (!std::filesystem::exists(fullPath))
    {
        response = "HTTP/1.1 404 Not Found\r\n\r\n";
        return;
    }

    std::string fileContent;
    try {
        readFileContent(fullPath, fileContent);
        std::cout << "File content size: " << fileContent.size() << std::endl;
    } catch (const std::exception& e) {
        response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        return;
    }

    std::string mimeType = getMimeType(fullPath);

    response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + mimeType + "\r\n";
    response += "Content-Length: " + std::to_string(fileContent.size()) + "\r\n";
    response += "Cache-Control: max-age=3600\r\n";
    response += "\r\n" + fileContent;
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

    std::cout << "File content length: " << content.size() << std::endl;
}
