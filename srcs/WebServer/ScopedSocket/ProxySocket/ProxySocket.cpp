#include "ProxySocket.hpp"
#include <fcntl.h>

ProxySocket::ProxySocket(addrinfo* proxyInfo, const std::string& proxyHost)
    : ScopedSocket(socket(proxyInfo->ai_family, proxyInfo->ai_socktype, proxyInfo->ai_protocol), O_NONBLOCK | FD_CLOEXEC),
      _proxyHost(proxyHost)
{
    try
    {
        if (proxyInfo == nullptr)
            throw WebErrors::ProxyException("Invalid proxy info provided");
        setupSocketOptions();
        if (this->getFd() < 0 || connect(this->getFd() , proxyInfo->ai_addr, proxyInfo->ai_addrlen) < 0)
            throw WebErrors::ProxyException("Error connecting to proxy server");
    }
    catch (const WebErrors::ProxyException& e)
    {
        throw ;
    }
}

ProxySocket::~ProxySocket()
{
}

ProxySocket::ProxySocket(ProxySocket&& other) noexcept
    : ScopedSocket(std::move(other)),
      _proxyHost(std::move(other._proxyHost))
{
}

void ProxySocket::setupSocketOptions()
{
    int flag = 1;
    if (setsockopt(this->getFd(), IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0)
        throw WebErrors::ProxyException("Error setting TCP_NODELAY");
}

const std::string& ProxySocket::getProxyHost() const
{
    return _proxyHost;
}
