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
#include "WebServer.hpp"

Response::Response(const Request &request)
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
            ErrorHandler(request.getServer()).handleError(response, request.getErrorCode());
            return response;
        }
        else if (request.getLocation()->type == LocationType::HTTP_REDIR)
        {
            const std::string& redirectUrl = request.getLocation()->target;
            response += "HTTP/1.1 302 Found\r\n";
            response += "Location: " + redirectUrl + "\r\n";
            response += "Content-Length: 0\r\n";
            response += "\r\n";
            return response;
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::STANDARD
            || request.getLocation()->type == LocationType::ALIAS)
        {
            StaticFileHandler(request).serveFile(response);
        }
        if (request.getRequestData().method == "HEAD" && request.getErrorCode() == 0)
        {
            size_t headerEndPos = response.find("\r\n\r\n");
            if (headerEndPos != std::string::npos)
                response = response.substr(0, headerEndPos + 4);
        }
        return response;
    }
    catch (const std::exception &e)
    {
        WebErrors::printerror("Response::generate", e.what());
        throw;
    }
}


const std::string &Response::getResponse() const
{
    return _response;
}
