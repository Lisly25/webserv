#include "Response.hpp"
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
            std::cout << request.getRequestData().uri << std::endl;
            std::cout << request.getRequestData().uri << std::endl;
            std::cout << request.getRequestData().uri << std::endl;
            std::cout << request.getRequestData().uri << std::endl;
        
        std::cout << "Location type: " << request.getLocation()->type << std::endl;
                std::cout << "Location type: " << request.getLocation()->type << std::endl;
                        std::cout << "Location type: " << request.getLocation()->type << std::endl;
                                std::cout << "Location type: " << request.getLocation()->type << std::endl;
        std::string response;
        if (request.getErrorCode() != 0)
        {
            ErrorHandler(request).handleError(response);
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::CGI)
        {
            // handleCGI(request, response);
        }
        else if (request.getLocation()->type == LocationType::ALIAS)
        {
            // handleAlias(request, response);
        }
        else
        {
            std::cout << "Static file handler" << std::endl;
            StaticFileHandler(request).serveFile(response);
            //std::cout << response << std::endl;
        }
        return response;
    }
    catch (const std::exception &e)
    {
        throw ;
    }
}

const std::string &Response::getResponse() const
{
    return _response;
}
