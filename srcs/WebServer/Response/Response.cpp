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
            ErrorHandler(request).handleError(response, request.getErrorCode());
            return response;
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::CGI)
        {
            CGIHandler cgi = CGIHandler(request);
            response += cgi.getCGIResponse();
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
        std::cout << "Error in Response::generate\n";
        throw;
    }
}


const std::string &Response::getResponse() const
{
    return _response;
}
