#include "Request.hpp"
#include <iostream>
#include <sys/stat.h> 
#include <filesystem>

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
                        _request._errorCode = INVALID_METHOD;
                    }
                    if (!isPathValid())
                    {
                        _request._errorCode = NOT_FOUND;
                    }
                    if (!isProtocolValid())
                    {
                        _request._errorCode = HTTP_VERSION_NOT_SUPPORTED;
                    }

                    if (!areHeadersValid())
                    {
                        _request._errorCode = BAD_REQUEST;
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

    return  ((method == "GET" && _request._location->allowedGET)
        || (method == "POST" && _request._location->allowedPOST)
        || (method == "DELETE" && _request._location->allowedDELETE));
}

bool Request::RequestValidator::isPathValid() const
{
    std::string relativeUri = _request._requestData.uri;
    if (relativeUri.find(_request._location->uri) == 0)
        relativeUri = relativeUri.substr(_request._location->uri.length());
    if (!relativeUri.empty() && relativeUri.front() != '/')
        relativeUri = "/" + relativeUri;

    std::string fullPath = std::filesystem::current_path().generic_string() + _request._location->root + relativeUri;
    if (std::filesystem::is_directory(fullPath))
    {
        if (_request._location->autoIndexOn)
            std::cout << "Autoindex enabled, serving directory listing" << std::endl;
        else
        {
            bool indexFound = false;
            for (const auto& indexFile : _request._location->index)
            {
                std::string indexPath = fullPath + "/" + indexFile;
                std::cout << "Checking index file: " << indexPath << std::endl;

                if (std::filesystem::exists(indexPath))
                {
                    fullPath = indexPath;
                    indexFound = true;
                    break;
                }
            }
            if (!indexFound)
            {
                std::cout << "No index file found and autoindex is off. Path is invalid." << std::endl;
                return false;
            }
        }
    }
    fullPath = std::filesystem::absolute(fullPath).generic_string();
    std::cout << "Checking fullPath: " << fullPath << std::endl;

    _request._requestData.uri = fullPath;
    return std::filesystem::exists(fullPath);
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

