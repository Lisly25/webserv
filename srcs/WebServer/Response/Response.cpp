#include "Response.hpp"
#include "Request.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>

std::string Response::generate(const Request &request)
{
    std::string response;

    // Check if the request is proxied
    if (request.getLocationType() == LocationType::PROXY)
    {
        handleProxyPass(request, response);
    }
    else if (request.getLocationType() == LocationType::CGI)
    {
        // handleCGI(request, response);
    }
    else if (request.getLocationType() == LocationType::ALIAS)
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

    const int proxySocket = socket(proxyInfo->ai_family, proxyInfo->ai_socktype, proxyInfo->ai_protocol);
    if (proxySocket < 0 || connect(proxySocket, proxyInfo->ai_addr, proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    std::cout << "Forwarding request to proxy:\n" << request.getRawRequest() << std::endl;

    std::string modifiedRequest = request.getRawRequest();
    size_t hostPos = modifiedRequest.find("Host: ");
    if (hostPos != std::string::npos) {
        size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
        if (hostEnd != std::string::npos)
            modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), "localhost:4141");
    }

    std::cout << "Modified request:\n" << modifiedRequest << std::endl;

    if (send(proxySocket, modifiedRequest.c_str(), modifiedRequest.length(), 0) < 0)
        throw WebErrors::ProxyException("Error sending to proxy server");

    // Wait for the response from the proxy server
    char buffer[4096];
    ssize_t bytesRead = 0;
    while ((bytesRead = recv(proxySocket, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytesRead);
        std::cout << "Received response chunk from proxy, size: " << bytesRead << std::endl;
    }

    if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw WebErrors::ProxyException("Error reading from proxy server");

    std::cout << "Full response from proxy:\n" << response << std::endl;

    close(proxySocket);
}