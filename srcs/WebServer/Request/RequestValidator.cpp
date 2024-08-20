#include "Request.hpp"
#include <iostream>

Request::RequestValidator::RequestValidator(Request& request, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _request(request), _servers(servers), _proxyInfoMap(proxyInfoMap) {}

bool Request::RequestValidator::validate() const
{
    for (const auto& srv : _servers)
    {
        if (isServerMatch(srv))
        {
            if (matchLocationSetData(srv))
            {
                _request._server = &srv;
                return true;
            }
        }
    }
    return false;
}

bool Request::RequestValidator::isServerMatch(const Server& server) const
{
    size_t hostPos = _request._rawRequest.find("Host: ");
    if (hostPos == std::string::npos)
        return false;

    size_t hostEnd = _request._rawRequest.find("\r\n", hostPos);
    std::string hostHeader = _request._rawRequest.substr(hostPos + 6, hostEnd - (hostPos + 6));

    for (const auto& serverName : server.server_name)
    {
        std::string fullServerName = serverName + ":" + std::to_string(server.port);
        if (hostHeader == fullServerName)
        {
            _request._server = &server;
            return true;
        }
    }
    return false;
}

bool Request::RequestValidator::matchLocationSetData(const Server& server) const
{
    const Location* bestMatchLocation = nullptr;
    std::string uri = _request.extractUri(_request._rawRequest);

    for (const auto& location : server.locations)
    {
        if (uri.find(location.uri) == 0)
        {
            if (!bestMatchLocation || location.uri.length() > bestMatchLocation->uri.length())
                bestMatchLocation = &location;
        }
    }

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
        _request._location = bestMatchLocation;

        if (bestMatchLocation->type == PROXY)
        {
            auto proxyInfoIt = _proxyInfoMap.find(bestMatchLocation->target);
            if (proxyInfoIt != _proxyInfoMap.end())
                _request._proxyInfo = proxyInfoIt->second;
        }
        return true;
    }
    return false;
}
