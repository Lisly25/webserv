#include "Request.hpp"
#include <iostream>
#include <algorithm>

Request::Request()
    : _rawRequest(""), _server(nullptr), _location(nullptr), _proxyInfo(nullptr)
{
}

Request::Request(const std::string& rawRequest, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _rawRequest(rawRequest), _server(nullptr), _location(nullptr), _proxyInfo(nullptr)
{
    initialize(servers, proxyInfoMap);
    if (!_server || !_location)
        throw std::runtime_error("No matching  Server or location found for request");
}

void Request::initialize(const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    const std::string requestLine = _rawRequest.substr(0, _rawRequest.find("\r\n"));
    const std::string uri = extractUri(requestLine);

    for (const auto& server : servers)
    {
        if (isServerMatch(server))
        {
            if (matchLocationSetData(server, uri, proxyInfoMap))
                return;
        }
    }
}

std::string Request::extractUri(const std::string& requestLine) const
{
    std::string uri = requestLine.substr(requestLine.find(" ") + 1);
    uri = uri.substr(0, uri.find(" "));  // Extract URI from "GET /uri HTTP/1.1"
    //std::cout << "Extracted URI: " << uri << std::endl;
    return uri;
}

bool Request::isServerMatch(const Server& server)
{
    // Extract the Host header from the raw request
    size_t hostPos = _rawRequest.find("Host: ");
    if (hostPos == std::string::npos)
        return false;

    size_t hostEnd = _rawRequest.find("\r\n", hostPos);
    std::string hostHeader = _rawRequest.substr(hostPos + 6, hostEnd - (hostPos + 6));

    // Iterate through the server names to see if any match the Host header
    for (const auto& serverName : server.server_name)
    {
        std::string fullServerName = serverName + ":" + std::to_string(server.port);
        std::cout << "Checking Server Name: " << fullServerName << std::endl;
        std::cout << "Host Header: " << hostHeader << std::endl;
        if (hostHeader == fullServerName)
        {
            std::cout << "Extracted Host Header: " << hostHeader << std::endl;
            std::cout << "Checking Server:" << std::endl;
            std::cout << "Host: " << server.host << std::endl;
            std::cout << "Server match for host " << server.host << ": true" << std::endl;
            _server = &server;
            return true;
        }
    }

    std::cout << "Extracted Host Header: " << hostHeader << std::endl;
    std::cout << "No matching server name found for host: " << hostHeader << std::endl;
    return false;
}



bool Request::matchLocationSetData(const Server& server, const std::string& uri, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    const Location* bestMatchLocation = nullptr;

    for (const auto& location : server.locations)
    {
        //std::cout << "Checking Location: " << location.uri << " against URI: " << uri << std::endl;

        if (uri.find(location.uri) == 0)  // Matches the beginning of the URI
        {
            if (!bestMatchLocation || location.uri.length() > bestMatchLocation->uri.length())
            {
                bestMatchLocation = &location;  // Prefer longer matches
            }
        }
    }

    // If no specific location matched, fall back to root location "/"
    if (!bestMatchLocation)
    {
        for (const auto& location : server.locations)
        {
            if (location.uri == "/")
            {
                bestMatchLocation = &location;
                break;
            }
        }
    }

    if (bestMatchLocation)
    {
        std::cout << "Setting Location: " << bestMatchLocation->uri << std::endl;
        _location = bestMatchLocation;
        if (_location->type == PROXY)
        {
            setProxyInfo(proxyInfoMap);
        }

        return true;
    }
    return false;
}



void Request::setProxyInfo(const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
{
    std::cout << "Target: " << _location->target << std::endl;
    std::string key = _location->target;
    auto proxyInfoIt = proxyInfoMap.find(key);
    if (proxyInfoIt != proxyInfoMap.end())
    {
        std::cout << "Found Setting Proxy Info\n\n" << std::endl;
        _proxyInfo = proxyInfoIt->second;
    }
}

const std::string& Request::getRawRequest() const { return _rawRequest; }

const Server* Request::getServer() const { return _server; }

const Location* Request::getLocation() const { return _location; }

addrinfo* Request::getProxyInfo() const { return _proxyInfo; }
