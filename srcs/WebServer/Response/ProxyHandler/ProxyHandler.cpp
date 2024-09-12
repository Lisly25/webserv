#include "ProxyHandler.hpp"
#include "ProxySocket/ProxySocket.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>

ProxyHandler::ProxyHandler(const Request& req) : _request(req), _proxyInfo(req.getProxyInfo()), _proxyHost(req.getLocation()->target)
{
    if (!_proxyInfo) throw WebErrors::ProxyException("No proxy information available");
}

bool ProxyHandler::isDataAvailable(int fd, int timeout_usec)
{
    if (fd < 0)
        throw WebErrors::ProxyException("Invalid file descriptor.");
    try
    {
        struct timeval timeout;
        timeout.tv_sec = timeout_usec / 1000000;
        timeout.tv_usec = timeout_usec % 1000000;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        int ret = select(fd + 1, &readfds, nullptr, nullptr, &timeout);

        if (ret < 0) {
            throw WebErrors::ProxyException("Error with select on proxy server socket");
        }
        return ret > 0 && FD_ISSET(fd, &readfds);
    }
    catch (const std::exception &e)
    {
        WebErrors::printerror("ProxyHandler::isDataAvailable", e.what());
        throw;
    }
}

std::string ProxyHandler::modifyRequestForProxy()
{
    try
    {
        std::string modifiedRequest = _request.getRawRequest();

        auto replaceHostHeader = [&](const std::string& newHost) {
            size_t hostPos = modifiedRequest.find("Host: ");
            if (hostPos != std::string::npos)
            {
                size_t hostEnd = modifiedRequest.find("\r\n", hostPos);
                if (hostEnd != std::string::npos)
                    modifiedRequest.replace(hostPos + 6, hostEnd - (hostPos + 6), newHost);
            }
        };

        auto modifyUri = [&]() {
            std::string locationUri = _request.getLocation()->uri;
            size_t uriPos = modifiedRequest.find(locationUri);
            if (uriPos != std::string::npos && locationUri != "/")
            {
                std::string newUri = modifiedRequest.substr(uriPos + locationUri.length());
                if (newUri.empty() || newUri[0] != '/')
                    newUri = "/" + newUri;
                modifiedRequest.replace(uriPos, modifiedRequest.find(" ", uriPos) - uriPos, newUri);
            }
        };

        replaceHostHeader(_proxyHost);
        modifyUri();

        return modifiedRequest;
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

void ProxyHandler::passRequest(std::string &response)
{
    try {
        ProxySocket proxySocket(_proxyInfo, _proxyHost);
        std::string modifiedRequest = modifyRequestForProxy();
        char        buffer[8192];
        ssize_t     bytesRead = 0;

        if (send(proxySocket.getFd(), modifiedRequest.c_str(), modifiedRequest.length(), 0) < 0)
            throw WebErrors::ProxyException("Error sending to proxy server");

        while (isDataAvailable(proxySocket.getFd(), 20000)) // 2ms timeout
        {
            bytesRead = recv(proxySocket.getFd(), buffer, sizeof(buffer), 0);
            if (bytesRead > 0)
            {
                response.append(buffer, bytesRead);
            }
            else if (bytesRead == 0)
                break;
            else if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                throw WebErrors::ProxyException("Error reading from proxy server");
        }
    }
    catch (const std::exception &e)
    {
        throw ;
    }
}
