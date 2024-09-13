#pragma once

#include "ScopedSocket.hpp"
#include "Request.hpp"
#include "WebServer.hpp"
#include <string>

class ProxyHandler
{
public:
    ProxyHandler(const Request& request, WebServer &webServer);
    ~ProxyHandler();
    void            passRequest(std::string &response);
    void            handleProxyWrite(int proxySocketFd);
    void            passRequest();
    void            handleProxyRead(int proxySocketFd);
    int            getProxySocketFd() const;

private:
    WebServer       &_webServer;
    const Request&  _request;
    addrinfo*       _proxyInfo;
    std::string     _proxyHost;
    int             _proxySocketFd = -1;
    int            _clientSocketFd = -1;

    std::string    _pendingProxyRequest;
    std::string    _pendingProxyResponse;

    void            closeProxyConnection(int proxySocketFd);
    void            sendRequestToProxy(ScopedSocket& proxySocket, const std::string& modifiedRequest);
    bool            isDataAvailable(int fd, int timeout_usec);
    std::string     modifyRequestForProxy();
};
