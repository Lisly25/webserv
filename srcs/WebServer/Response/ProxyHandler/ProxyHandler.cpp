#include "ProxyHandler.hpp"
#include "ProxySocket/ProxySocket.hpp"
#include "WebErrors.hpp"
#include <cstring>
#include <netinet/tcp.h>
#include <unistd.h>
#include <iostream>

ProxyHandler::ProxyHandler(const Request& req, WebServer &webServer) 
    : _webServer(webServer), 
      _request(req), 
      _proxyInfo(req.getProxyInfo()), 
      _proxyHost(req.getLocation()->target)
{
    if (!_proxyInfo)
        throw WebErrors::ProxyException("No proxy information available");
    try
    {
        ProxySocket proxySocket(_proxyInfo, _proxyHost);

        _proxySocketFd = proxySocket.getFd();
        proxySocket.release(false);
        _pendingProxyRequest = modifyRequestForProxy();
        _webServer.epollController(_proxySocketFd, EPOLL_CTL_ADD, EPOLLOUT, FdType::PROXY_FD);
    } 
    catch (const std::exception &e)
    {
        throw;
    }
}

ProxyHandler::~ProxyHandler()
{
}


void ProxyHandler::closeProxyConnection(int proxySocketFd)
{
    _webServer.epollController(proxySocketFd, EPOLL_CTL_DEL, 0, FdType::PROXY_FD);
    _webServer.getProxyHandlerMap().erase(proxySocketFd);
}

void ProxyHandler::handleProxyWrite(int proxySocketFd)
{
    ssize_t bytesSent = send(proxySocketFd, _pendingProxyRequest.c_str(), _pendingProxyRequest.length(), 0);

    if (bytesSent > 0)
    {
        _pendingProxyRequest.erase(0, bytesSent);
        if (_pendingProxyRequest.empty())
            _webServer.epollController(proxySocketFd, EPOLL_CTL_MOD, EPOLLIN, FdType::PROXY_FD);
    }
    else if (bytesSent == -1)
    {
        closeProxyConnection(proxySocketFd);
    }
}

void ProxyHandler::handleProxyRead(int proxySocketFd)
{
    char    buffer[4096];
    ssize_t bytesReceived = recv(proxySocketFd, buffer, sizeof(buffer), 0);
    ssize_t bytesSent = 0;

    if (bytesReceived > 0)
    {
        _pendingProxyResponse.append(buffer, bytesReceived);
        bytesSent = send(_clientSocketFd, _pendingProxyResponse.c_str(), _pendingProxyResponse.size(), 0);
        if (bytesSent > 0)
            _pendingProxyResponse.erase(0, bytesSent);
        if (bytesReceived == 0 || _pendingProxyResponse.empty())
            closeProxyConnection(proxySocketFd);
    }
    else if (bytesReceived == 0)
        closeProxyConnection(proxySocketFd);
    else if (bytesReceived == -1)
        closeProxyConnection(proxySocketFd);
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

int ProxyHandler::getProxySocketFd() const { return _proxySocketFd; }
