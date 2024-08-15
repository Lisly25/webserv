#include "ProxyHandler.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>

ProxyHandler::ProxyHandler(const Request& req) : request(req), proxyInfo(req.getProxyInfo()), proxyHost(req.getLocation()->target)
{
    if (!proxyInfo) throw WebErrors::ProxyException("No proxy information available");
}

ScopedSocket ProxyHandler::createProxySocket(addrinfo* proxyInfo)
{
    ScopedSocket proxySocket(socket(proxyInfo->ai_family, proxyInfo->ai_socktype, proxyInfo->ai_protocol));
    
    int flag = 1;
    if (setsockopt(proxySocket.get(), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
        throw WebErrors::ProxyException("Error setting TCP_NODELAY");

    if (proxySocket.get() < 0 || connect(proxySocket.get(), proxyInfo->ai_addr, proxyInfo->ai_addrlen) < 0)
        throw WebErrors::ProxyException("Error connecting to proxy server");

    return proxySocket;
}

bool ProxyHandler::isDataAvailable(int fd, int timeout_usec)
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

std::string ProxyHandler::modifyRequestForProxy()
{
    std::string modifiedRequest = request.getRawRequest();
    size_t      hostPos = modifiedRequest.find("Host: ");
    
    if (hostPos != std::string::npos)
    {
        size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
        if (hostEnd != std::string::npos)
            modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), proxyHost);
    }
    std::cout << "Modified request for proxy: " << modifiedRequest << std::endl;
    return modifiedRequest;
}

void ProxyHandler::passRequest(std::string &response)
{
    try {
        ScopedSocket proxySocket = createProxySocket(proxyInfo);
        std::string modifiedRequest = modifyRequestForProxy();
        char        buffer[8192];
        ssize_t     bytesRead = 0;

        std::cout << "Sending request to proxy: " << proxyHost << std::endl;
        if (send(proxySocket.get(), modifiedRequest.c_str(), modifiedRequest.length(), 0) < 0)
            throw WebErrors::ProxyException("Error sending to proxy server");

        while (isDataAvailable(proxySocket.get(), 500000)) // 50ms timeout
        {
            bytesRead = recv(proxySocket.get(), buffer, sizeof(buffer), 0);
            if (bytesRead > 0)
            {
                response.append(buffer, bytesRead);
                std::cout << "Received response from proxy: " << proxyHost << "\n" ;
            }
            else if (bytesRead == 0)
            {
                std::cout << "Proxy server closed the connection." << std::endl;
                break;
            }
            else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                throw WebErrors::ProxyException("Error reading from proxy server");
        }
        //std::cout << "Final received data from proxy: " << proxyHost << "  " << response << std::endl;
    }
    catch (const std::exception &e)
    {
        throw WebErrors::ProxyException(e.what());
    }
}
