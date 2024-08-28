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
            std::cout << "\033[35mURI: \033[0m" << request.getRequestData().uri << std::endl;
            std::cout << "\033[35mTARGET: \033[0m" << request.getLocation()->target << std::endl;
            /*std::cout << request.getRequestData().uri << std::endl;*/
            /*std::cout << request.getRequestData().uri << std::endl;*/
            /*std::cout << request.getRequestData().uri << std::endl;*/
        
        std::cout << "Location type: " << request.getLocation()->type << std::endl;
                /*std::cout << "Location type: " << request.getLocation()->type << std::endl;*/
                /*        std::cout << "Location type: " << request.getLocation()->type << std::endl;*/
                /*                std::cout << "Location type: " << request.getLocation()->type << std::endl;*/
        std::string response;
        if (request.getErrorCode() != 0)
        {
            std::cout << "\033[31mCGI NOT going to RUN\033[0m\n";
            ErrorHandler(request).handleError(response);
        }
        else if (request.getLocation()->type == LocationType::PROXY)
        {
            ProxyHandler(request).passRequest(response);
        }
        else if (request.getLocation()->type == LocationType::CGI)
        {
            std::cout << "CGI PASSING" << std::endl;
            std::cout << request.getRequestData().query_string << std::endl;
            CGIHandler cgi = CGIHandler(request);
            response = cgi.getCGIResponse();
            std::cout << "\033[31mresponse from CGI: \033[0m" << response << std::endl;
        }
        else if (request.getLocation()->type == LocationType::STANDARD
            || request.getLocation()->type == LocationType::ALIAS)
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
