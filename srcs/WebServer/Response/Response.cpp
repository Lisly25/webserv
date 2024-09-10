#include "Response.hpp"
#include "CGIHandler/CGIHandler.hpp"
#include "ErrorHandler/ErrorHandler.hpp"
#include "ProxyHandler/ProxyHandler.hpp"
#include "Request.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "StaticFileHandler.hpp"

Response::Response(const Request &request, WebServer &webServer) : _response(""), _webServer(webServer)
{
    try {
        _response = generate(request);
    }
    catch (const std::exception &e)
    {
        throw ;
    }
}

std::string Response::generate(const Request &request)
{
    try {
        std::string response;

        if (request.getErrorCode() != 0)
        {
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            ErrorHandler(request).handleError(response);
            return response;
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::CGI)
        {   
            CgiInfo cgiInfo = CGIHandler(request).executeScript();
            if (cgiInfo.error)
                return (WebParser::getErrorPage(500, request.getServer()));
            cgiInfo.clientSocket = _webServer.getCurrentEventFd();
            _webServer.epollController(cgiInfo.readEndFd, EPOLL_CTL_ADD, EPOLLIN);
            _webServer.getCgiInfos().push_back(cgiInfo);
        }
        else if (request.getLocation()->type == LocationType::STANDARD || request.getLocation()->type == LocationType::ALIAS)
        {
            StaticFileHandler(request).serveFile(response);
        }
        if (request.getRequestData().method == "HEAD" && request.getErrorCode() == 0)
        {
            std::cout << "Response before HEAD: " << response << std::endl;
            size_t headerEndPos = response.find("\r\n\r\n");
            if (headerEndPos != std::string::npos)
                response = response.substr(0, headerEndPos + 4);
            std::cout << "HEAD response: " << response << std::endl;
        }

        return response;
    }
    catch (const std::exception &e)
    {
        std::cout << "Error in Response::generate\n";
        throw;
    }
}

const std::string &Response::getResponse() const
{
    return _response;
}
