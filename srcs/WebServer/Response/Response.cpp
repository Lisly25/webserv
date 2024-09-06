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
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            std::cout << "Error code: " << request.getErrorCode() << std::endl;
            ErrorHandler(request).handleError(response);
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
