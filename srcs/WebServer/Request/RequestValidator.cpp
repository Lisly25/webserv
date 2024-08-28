#include "Request.hpp"
#include <iostream>
#include <sys/stat.h> 
#include <filesystem>

Request::RequestValidator::RequestValidator(Request& request, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _request(request), _servers(servers), _proxyInfoMap(proxyInfoMap) {}

bool Request::RequestValidator::validate() const
{
    try
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
                        /*if (!isValidMethod())*/
                        /*{*/
                        /*    std::cout << "here1\n";*/
                        /*    _request._errorCode = INVALID_METHOD;*/
                        /*}*/
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
                    std::cout << "ERROR CODE VALIDATION: " << _request._errorCode << std::endl;
                    return true;
                }
            }
        }
        return false;
    }
    catch (const std::exception& e)
    {
        throw ;
    }
}

bool Request::RequestValidator::isValidMethod() const
{
    try
    {
        const std::string& method = _request._requestData.method;

        return  ((method == "GET" && _request._location->allowedGET)
            || (method == "POST" && _request._location->allowedPOST)
            || (method == "DELETE" && _request._location->allowedDELETE));
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error validating method: ") + e.what());
    }
}

bool Request::RequestValidator::checkForIndexing(std::string& fullPath) const
{
    try
    {
        if (std::filesystem::is_directory(fullPath))
        {
            if (_request._location->autoIndexOn)
            {
                std::cout << "Autoindex enabled, serving directory listing" << std::endl;
                return true;
            }
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
        return true;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error checking indexing: ") + e.what());
    }
}

bool Request::RequestValidator::isPathValid() const
{
    try
    {
        std::string relativeUri = _request._requestData.uri;

        auto handleAlias = [&]() -> bool {
            if (relativeUri.find(_request._location->uri) == 0)
                relativeUri = relativeUri.substr(_request._location->uri.length());
            if (!relativeUri.empty() && relativeUri.front() != '/')
                relativeUri = "/" + relativeUri;
            std::string fullPath = _request._location->target + relativeUri;
            fullPath = std::filesystem::absolute(fullPath).generic_string();
            std::cout << "Alias fullPath: " << fullPath << std::endl;
            if (!checkForIndexing(fullPath))
                return false;

            _request._requestData.uri = fullPath;
            return std::filesystem::exists(fullPath);
        };

        auto handleRoot = [&]() -> bool {
            if (relativeUri.find(_request._location->uri) == 0)
                relativeUri = relativeUri.substr(_request._location->uri.length());
            if (!relativeUri.empty() && relativeUri.front() != '/')
                relativeUri = "/" + relativeUri;
            std::string fullPath = _request._location->root + _request._location->uri + relativeUri;
            if (!checkForIndexing(fullPath))
                return false;
            fullPath = std::filesystem::absolute(fullPath).generic_string();
            std::cout << "Checking fullPath: " << fullPath << std::endl;
            _request._requestData.uri = fullPath;
            return std::filesystem::exists(fullPath);
        };

             auto handleCGIPass = [&]() -> bool {
            std::string fullPath = _request._location->target;
            std::string::size_type queryPos = fullPath.find('?');
            if (queryPos != std::string::npos) {
                fullPath = fullPath.substr(0, queryPos);
            }
            fullPath = "." + fullPath;
            fullPath = std::filesystem::absolute(fullPath).generic_string();
            std::cout << "CGI fullPath: " << fullPath << std::endl;
            _request._requestData.uri = fullPath;
            return std::filesystem::exists(fullPath);
        };



        if (_request._location->type == ALIAS)
            return handleAlias();
        else if (_request._location->type == CGI)
            return handleCGIPass();
        else
            return handleRoot();
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error validating path: ") + e.what());
    }
}

bool Request::RequestValidator::isProtocolValid() const
{
    try
    {
        return _request._requestData.httpVersion == "HTTP/1.1";
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error validating protocol: ") + e.what());
    }
}

bool Request::RequestValidator::areHeadersValid() const
{
    try
    {
        if (_request._requestData.headers.find("Host") == _request._requestData.headers.end())
            return false;
        return true;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error validating headers: ") + e.what());
    }
}

bool Request::RequestValidator::isServerMatch(const Server& server) const
{
    try
    {
        const auto& headers = _request._requestData.headers;
        auto hostIt = headers.find("Host");

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
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error matching server: ") + e.what());
    }
}

bool Request::RequestValidator::matchLocationSetData(const Server& server) const
{
    try
    {
        std::string uri = _request._requestData.uri;
        const Location* bestMatchLocation = nullptr;

        for (const auto& location : server.locations)
        {
            if (uri.find(location.uri) == 0)
            {
                if (!bestMatchLocation || location.uri.length() > bestMatchLocation->uri.length())
                {
                    bestMatchLocation = &location;
                }
            }
        }

        if (!bestMatchLocation)
            return false;

        _request._location = bestMatchLocation;

        if (bestMatchLocation->type == PROXY)
        {
            auto it = _proxyInfoMap.find(bestMatchLocation->target);
            if (it != _proxyInfoMap.end())
            {
                _request._proxyInfo = it->second;
            }
        }

        return true;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error matching location and setting data: ") + e.what());
    }
}
