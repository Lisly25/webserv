#include "Response.hpp"
#include "Request.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>

std::string Response::generate(const Request &request)
{
    std::string response;

    // Check if the request is proxied
    if (request.getLocation()->type == LocationType::PROXY)
    {
        handleProxyPass(request, response);
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


void Response::handleProxyPass(const Request& request, std::string &response)
{
    addrinfo* proxyInfo = request.getProxyInfo();
    if (!proxyInfo)
        throw WebErrors::ProxyException("No proxy information available");

    // Use ScopedSocket to manage the proxy socket
    ScopedSocket proxySocket(socket(proxyInfo->ai_family, proxyInfo->ai_socktype, proxyInfo->ai_protocol), false);

    if (proxySocket.get() < 0 || connect(proxySocket.get(), proxyInfo->ai_addr, proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    std::string modifiedRequest = request.getRawRequest();

    // Use the target from the location directly
    const std::string& proxyHost = request.getLocation()->target;
    size_t hostPos = modifiedRequest.find("Host: ");
    if (hostPos != std::string::npos) {
        size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
        if (hostEnd != std::string::npos)
            modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), proxyHost);
    }

    if (send(proxySocket.get(), modifiedRequest.c_str(), modifiedRequest.length(), 0) < 0)
        throw WebErrors::ProxyException("Error sending to proxy server");

    // Wait for the response from the proxy server
    char buffer[4096];
    ssize_t bytesRead = 0;
    while ((bytesRead = recv(proxySocket.get(), buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytesRead);
    }

    if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw WebErrors::ProxyException("Error reading from proxy server");
}
