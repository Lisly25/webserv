#include "Response.hpp"
#include "ProxyHandler/ProxyHandler.hpp"
#include "Request.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>
#include <string>

std::string Response::generate(const Request &request)
{
    std::string response;

    if (request.getLocation()->type == LocationType::PROXY)
    {
        ProxyHandler    proxyHandler(request);
        proxyHandler.passRequest(response);
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
        // handleStandard(request, response);
    }
    return response;
}
