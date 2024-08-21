#include "Request.hpp"
#include <iostream>
#include <sys/stat.h> 

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

                if (_request._location->type != PROXY)
                {
                    if (!isValidMethod())
                    {
                        throw std::runtime_error("405 Method Not Allowed");
                    }
                    if (!isPathValid())
                    {
                        throw std::runtime_error("404 Not Found");
                    }
                    if (!isProtocolValid())
                    {
                        throw std::runtime_error("505 HTTP Version Not Supported");
                    }

                    if (!areHeadersValid())
                    {
                        throw std::runtime_error("400 Bad Request");
                    }

                }
                return true;
            }
        }
    }
    return false;
}


bool Request::RequestValidator::isValidMethod() const
{
    const std::string& method = _request._requestData.method;

    if ((method == "GET" && _request._location->allowedGET)
        || (method == "POST" && _request._location->allowedPOST)
        || (method == "DELETE" && _request._location->allowedDELETE))
    {
        return true;
    }
    
    return false;
}

bool Request::RequestValidator::isPathValid() const
{
    const std::string fullPath = _request._location->root + _request._requestData.uri;

    std::cout << "fullPath: " << fullPath << std::endl;
    auto fileExists = [](const std::string& path) -> bool {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    };
    return fileExists(fullPath);
}

bool Request::RequestValidator::isProtocolValid() const
{
    return _request._requestData.httpVersion == "HTTP/1.1";
}

bool Request::RequestValidator::areHeadersValid() const
{
    if (_request._requestData.headers.find("Host") == _request._requestData.headers.end())
        return false;
    return true;
}

bool Request::RequestValidator::isServerMatch(const Server& server) const
{
    const auto& headers = _request._requestData.headers;
    auto        hostIt = headers.find("Host");

    if (hostIt == headers.end()) return false;

    std::string hostHeader = WebParser::trimSpaces(hostIt->second);

    if (hostHeader.empty()) return false;

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
    std::string     uri = _request._requestData.uri;
    const Location* bestMatchLocation = nullptr;

    for (const auto& location : server.locations)
    {
        if (uri.find(location.uri) == 0 && 
            (!bestMatchLocation || location.uri.length() > bestMatchLocation->uri.length()))
            bestMatchLocation = &location;
        else if (location.uri == "/" && !bestMatchLocation)
            bestMatchLocation = &location;
    }
    if (!bestMatchLocation)
        return false;
    _request._location = bestMatchLocation;
    if (bestMatchLocation->type == PROXY)
    {
        if (auto it = _proxyInfoMap.find(bestMatchLocation->target); it != _proxyInfoMap.end())
            _request._proxyInfo = it->second;
    }
    return true;
}

