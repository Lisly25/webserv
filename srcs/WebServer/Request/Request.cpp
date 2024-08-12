#include "Request.hpp"
#include <iostream>
#include <algorithm>

Request::Request()
    : _rawRequest(""), _server(nullptr), _location(nullptr), _isProxied(false), _proxyInfo(nullptr)
{
}

Request::Request(const std::string& rawRequest, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _rawRequest(rawRequest), _server(nullptr), _location(nullptr), _isProxied(false), _proxyInfo(nullptr)
{
    initialize(servers, proxyInfoMap);
}

void Request::initialize(const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    const std::string requestLine = _rawRequest.substr(0, _rawRequest.find("\r\n"));
    const std::string uri = extractUri(requestLine);

    for (const auto& server : servers)
    {
        if (isServerMatch(server))
        {
            if (matchLocationAndSetProxy(server, uri, proxyInfoMap))
                return;
        }
    }
}

std::string Request::extractUri(const std::string& requestLine) const
{
    std::string uri = requestLine.substr(requestLine.find(" ") + 1);
    uri = uri.substr(0, uri.find(" "));  // Extract URI from "GET /uri HTTP/1.1"
    std::cout << "Extracted URI: " << uri << std::endl;
    return uri;
}

bool Request::isServerMatch(const Server& server) const
{
    // Extract Host header from the raw request
    size_t hostPos = _rawRequest.find("Host: ");
    if (hostPos == std::string::npos)
        return false;

    size_t hostEnd = _rawRequest.find("\r\n", hostPos);
    std::string hostHeader = _rawRequest.substr(hostPos + 6, hostEnd - (hostPos + 6));

    bool match = hostHeader == server.host;
    std::cout << "Extracted Host Header: " << hostHeader << std::endl;
    std::cout << "Checking Server:" << std::endl;
    std::cout << "Host: " << server.host << std::endl;
    std::cout << "Server match for host " << server.host << ": " << match << std::endl;

      std::cout << "HERE server location uri: " << server.locations[0].uri << std::endl;
    std::cout << "HERE locatio ntype: " << server.locations[0].type << std::endl;

    return true;
}



bool Request::matchLocationAndSetProxy(const Server& server, const std::string& uri, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    const Location* rootLocation = nullptr;

      std::cout << "server location uri: " << server.locations[0].uri << std::endl;
    std::cout << "locatio ntype: " << server.locations[0].type << std::endl;


    for (const auto& location : server.locations)
    {
        std::cout << "Checking Location: " << location.uri << " against URI: " << uri << std::endl;
        if (location.uri == "/")
        {
            rootLocation = &location;
        }
        if (uri.find(location.uri) == 0 && location.uri != "/")
        {
            std::cout << "Match found for Location: " << location.uri << std::endl;
            _server = &server;
            _location = &location;
            _isProxied = (_location->type == PROXY);
            if (_isProxied)
                setProxyInfo(proxyInfoMap);
            return true;
        }
    }

    // Fallback to root location if no other location matches
    if (rootLocation)
    {
        std::cout << "Fallback to root Location: " << rootLocation->uri << std::endl;
        _server = &server;
        _location = rootLocation;
        _isProxied = (_location->type == PROXY);
        if (_isProxied)
            setProxyInfo(proxyInfoMap);
        return true;
    }

    return false;
}



void Request::setProxyInfo(const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    std::cout << "Target: " << _location->target << std::endl;
    std::string key = _location->target;//+ ":" + std::to_string(_server->port);
    auto proxyInfoIt = proxyInfoMap.find(key);
    if (proxyInfoIt != proxyInfoMap.end())
        _proxyInfo = proxyInfoIt->second;
}

const std::string& Request::getRawRequest() const { return _rawRequest; }

const Server* Request::getServer() const { return _server; }

bool Request::isProxied() const { return _isProxied; }

addrinfo* Request::getProxyInfo() const { return _proxyInfo; }
