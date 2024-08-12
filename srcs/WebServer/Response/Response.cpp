#include "Response.hpp"
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

bool Response::isDataAvailable(int fd, int timeout_usec)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_usec;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    int ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
    if (ret < 0)
        throw WebErrors::ProxyException("Error with select on proxy server socket");
    return ret > 0 && FD_ISSET(fd, &readfds);
}

void Response::handleProxyPass(const Request& request, std::string &response)
{
    addrinfo* proxyInfo = request.getProxyInfo();
    if (!proxyInfo) throw WebErrors::ProxyException("No proxy information available");

    ScopedSocket proxySocket = createProxySocket(proxyInfo);

    std::string modifiedRequest = request.getRawRequest();
    const std::string& proxyHost = request.getLocation()->target;

    size_t hostPos = modifiedRequest.find("Host: ");
    if (hostPos != std::string::npos)
    {
        size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
        if (hostEnd != std::string::npos)
            modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), proxyHost);
    }

    sendRequestToProxy(proxySocket, modifiedRequest);

    char buffer[8192];
    ssize_t bytesRead = 0;

    while (isDataAvailable(proxySocket.get(), 50000)) // 50ms timeout
    {
        bytesRead = recv(proxySocket.get(), buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            response.append(buffer, bytesRead);
            std::cout << "Received from proxy: " << proxyHost << "  " << response << std::endl;
        }
        else if (bytesRead == 0)
        {
            std::cout << "Proxy server closed the connection." << std::endl;
            break;
        }
        else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
            throw WebErrors::ProxyException("Error reading from proxy server");
    }

    std::cout << "Final received data from proxy: " << proxyHost << "  " << response << std::endl;
}

ScopedSocket Response::createProxySocket(addrinfo* proxyInfo)
{
    ScopedSocket proxySocket(socket(proxyInfo->ai_family, proxyInfo->ai_socktype, proxyInfo->ai_protocol), false);
    
    int flag = 1;
    if (setsockopt(proxySocket.get(), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
        throw WebErrors::ProxyException("Error setting TCP_NODELAY");

    if (proxySocket.get() < 0 || connect(proxySocket.get(), proxyInfo->ai_addr, proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    return proxySocket;
}

void Response::sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest)
{
    std::cout << "Sending to proxy: " << modifiedRequest << std::endl;
    if (send(proxySocket.get(), modifiedRequest.c_str(), modifiedRequest.length(), 0) < 0)
        throw WebErrors::ProxyException("Error sending to proxy server");
}
