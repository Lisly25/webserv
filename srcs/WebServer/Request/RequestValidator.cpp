#include "Request.hpp"
#include "WebErrors.hpp"
#include "WebServer.hpp"
#include <iostream>
#include <sys/stat.h> 
#include <filesystem>

Request::RequestValidator::RequestValidator(Request& request, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _request(request), _servers(servers), _proxyInfoMap(proxyInfoMap) {}

bool Request::RequestValidator::isReadOk() const
{
    if (access(_request._requestData.uri.c_str(), R_OK) == -1)
    {
        return false;
    }
    return true;
}

bool   Request::RequestValidator::isServerFull() const
{
    //the info returned is in bytes, same as our client max body size
    if (_request._requestData.method != "POST")
        return true;
    try
    {
        std::filesystem::space_info space = std::filesystem::space(_request._requestData.uri);
        if (space.available < 1048576)//an arbitrary number
            return false;
        if (_request._requestData.body.length() >= static_cast<size_t>(space.available))
            return false;
        return true;
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error(std::string("Error checking server's disk space: ") + e.what());
    }
}

bool   Request::RequestValidator::isExistingMethod() const
{
    std::string validMethods[] = {"GET", "POST", "DELETE", "HEAD"};

    for (size_t i = 0; i < 4; i++)
    {
        if (_request._requestData.method.compare(validMethods[i]) == 0)
            return true;
    }
    return false;
}

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
                    if (_request._requestData.uri.length() > 2048)
                    {
                        _request._errorCode = URI_TOO_LONG;
                        return true;
                    }
                    if (_request._location->type != PROXY && _request._location->type != HTTP_REDIR)
                    {
                        if (!isExistingMethod())
                        {
                            _request._errorCode = NOT_IMPLEMENTED;
                            return true;
                        }
                        if (!isAllowedMethod())
                        {
                            _request._errorCode = INVALID_METHOD;
                            return true;
                        }
                        if (_request.getServer()->client_max_body_size < static_cast<long>(_request._requestData.body.size()))
                        {
                            _request._errorCode = REQUEST_BODY_TOO_LARGE;
                            return true;
                        }
                        if (!isPathValid())
                        {
                            _request._errorCode = NOT_FOUND;
                            return true;
                        }
                        if (!isProtocolValid())
                        {
                            _request._errorCode = HTTP_VERSION_NOT_SUPPORTED;
                            return true;
                        }
                        if (!isReadOk())
                        {
                            _request._errorCode = FORBIDDEN;
                            return true;
                        }
                        if (!areHeadersValid())
                        {
                            _request._errorCode = BAD_REQUEST;
                            return true;
                        }
                        if (!isServerFull())
                        {
                            _request._errorCode = INSUFFICIENT_STORAGE;
                            return true;
                        }
                        if (_request._location->type == CGI)
                        {
                            if (!isUploadDirAccessible())
                            {
                                std::cerr << COLOR_RED_ERROR << \
                                    "  Error: no needed permissions for the cgi script to work on the upload folder\n\n" << COLOR_RESET;
                                _request._errorCode = FORBIDDEN;
                                return true;
                            }
                        }
                    }
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

bool Request::RequestValidator::isUploadDirAccessible() const
{
    std::string scriptPath = _request._requestData.uri;
    size_t      lastSlashPos = scriptPath.find_last_of('/');
    std::string uploadFolder = _request._location->upload_folder;
    std::string scriptDir;
    struct stat sb;

    if (lastSlashPos != std::string::npos)
        scriptDir = scriptPath.substr(0, lastSlashPos);
    else
        scriptDir = ".";

    if (uploadFolder.empty())
        uploadFolder = "uploads";

    const std::string uploadDirPath = scriptDir + "/" + uploadFolder;

    if (stat(uploadDirPath.c_str(), &sb) == 0)
    {
        if (S_ISDIR(sb.st_mode))
        {
            if (access(uploadDirPath.c_str(), W_OK) != 0 || access(uploadDirPath.c_str(), R_OK) != 0)
                return false;
            else
                return true;
        }
        else
            return false;
    }
    else
        return true;
}




bool Request::RequestValidator::isAllowedMethod() const
{
    try
    {
        const std::string& method = _request._requestData.method;

        return  ((method == "GET" && _request._location->allowedGET)
            || (method == "POST" && _request._location->allowedPOST)
            || (method == "DELETE" && _request._location->allowedDELETE)
            || (method == "HEAD" && _request._location->allowedHEAD));
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
                return true;
            }
            else
            {
                bool indexFound = false;
                for (const auto& indexFile : _request._location->index)
                {
                    std::string indexPath = fullPath + "/" + indexFile;

                    if (std::filesystem::exists(indexPath))
                    {
                        fullPath = indexPath;
                        indexFound = true;
                        break;
                    }
                }
                if (!indexFound)
                {
                    WebErrors::printerror("Request::RequestValidator::checkForIndexing", "No index file found");
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
        _request._requestData.originalUri = _request._requestData.uri;
        std::string relativeUri = _request._requestData.uri;

        auto handleAlias = [&]() -> bool {
            if (relativeUri.find(_request._location->uri) == 0)
                relativeUri = relativeUri.substr(_request._location->uri.length());
            if (!relativeUri.empty() && relativeUri.front() != '/')
                relativeUri = "/" + relativeUri;
            std::string fullPath = _request._location->target + relativeUri;
            fullPath = std::filesystem::absolute(fullPath).generic_string();
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
            fullPath = std::filesystem::canonical(fullPath).generic_string();
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
