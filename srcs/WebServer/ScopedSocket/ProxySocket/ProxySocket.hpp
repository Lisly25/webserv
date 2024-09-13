#pragma once

#include "ScopedSocket.hpp"
#include <netinet/tcp.h>
#include "WebErrors.hpp"

class ProxySocket : public ScopedSocket
{
public:
    ProxySocket(addrinfo* proxyInfo, const std::string& proxyHost);
    ProxySocket(ProxySocket&& other) noexcept;
    ProxySocket& operator=(ProxySocket&& other) noexcept = delete;
    ~ProxySocket();

    const std::string& getProxyHost() const;

private:
    std::string _proxyHost;
    void setupSocketOptions();
};
