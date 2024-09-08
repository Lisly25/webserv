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
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::CGI)
        {
            std::pair<pid_t, int> cgiInfo = CGIHandler(request).executeScript();
            if (cgiInfo.first != -1 && cgiInfo.second != -1)
            {
                CgiInfo cgiProcessInfo;
                cgiProcessInfo.pid = cgiInfo.first;
                cgiProcessInfo.readEndFd = cgiInfo.second;
                cgiProcessInfo.clientSocket = _webServer.getCurrentEventFd();
                cgiProcessInfo.isDone = false;

                _webServer.epollController(cgiProcessInfo.readEndFd, EPOLL_CTL_ADD, EPOLLIN);
                _webServer.getCgiFdMap().push_back(cgiProcessInfo);
            }
        }
        else if (request.getLocation()->type == LocationType::STANDARD || request.getLocation()->type == LocationType::ALIAS)
        {
            StaticFileHandler(request).serveFile(response);
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
