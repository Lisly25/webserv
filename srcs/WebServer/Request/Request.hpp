#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <netdb.h> 
#include "WebParser.hpp"

class Request
{
public:
    Request();
    Request(const std::string& rawRequest, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);

    const std::string&  getRawRequest() const;
    const Server*       getServer() const;
    const LocationType &getLocationType() const;
    addrinfo*           getProxyInfo() const;

private:
    std::string     _rawRequest;
    const Server*   _server;
    const Location* _location;
    LocationType    _locationType;
    addrinfo*       _proxyInfo;

    void        initialize(const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);
    std::string extractUri(const std::string& requestLine) const;
    bool        isServerMatch(const Server& server) const;
    bool        matchLocationAndSetProxy(const Server& server, const std::string& uri, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);
    void        setProxyInfo(const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);
};

