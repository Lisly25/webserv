// Response.cpp
#include "Response.hpp"
#include "Request.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>

Response::Response() : _proxyInfo(nullptr)
{
    resolveProxyAddresses();
}

Response::~Response()
{
    if (_proxyInfo)
    {
        freeaddrinfo(_proxyInfo);
        _proxyInfo = nullptr;
    }
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
    const int proxySocket = socket(_proxyInfo->ai_family, _proxyInfo->ai_socktype, _proxyInfo->ai_protocol);
    if (proxySocket < 0 || connect(proxySocket, _proxyInfo->ai_addr, _proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    std::cout << "Forwarding request to proxy:\n" << request << std::endl;

    // Modify the Host header to match the proxy host
    std::string modifiedRequest = request;
    size_t hostPos = modifiedRequest.find("Host: ");
    if (hostPos != std::string::npos) {
        size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
        if (hostEnd != std::string::npos) {
            modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), "localhost:4141");
        }
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


void Response::resolveProxyAddresses()
{
    const char* proxyHost = "localhost";
    const char* proxyPort = "4141";

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(proxyHost, proxyPort, &hints, &_proxyInfo);
    if (status != 0)
        throw WebErrors::ProxyException("Error resolving proxy address: " + std::string(gai_strerror(status)));
}
