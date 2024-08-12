#include "Response.hpp"
#include "Request.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>

Response::Response(addrinfo* proxyInfo) : _proxyInfo(proxyInfo)
{
}

Response::~Response()
{
    // The Response class should not free _proxyInfo since it is managed by WebServer
}

std::string Response::generate(const Request &request)
{
    std::string response;

    if (request.getRawRequest().find("GET /") != std::string::npos || request.getRawRequest().find("GET /proxy") != std::string::npos)
        handleProxyPass(request.getRawRequest(), response);
    else
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, this is the response from the server.\r\n";
    return response;
}


void Response::handleProxyPass(const std::string &request, std::string &response)
{
    if (!_proxyInfo)
        throw WebErrors::ProxyException("No proxy information available");

    const int proxySocket = socket(_proxyInfo->ai_family, _proxyInfo->ai_socktype, _proxyInfo->ai_protocol);
    if (proxySocket < 0 || connect(proxySocket, _proxyInfo->ai_addr, _proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    std::cout << "Forwarding request to proxy:\n" << request << std::endl;

    std::string modifiedRequest = request;
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
