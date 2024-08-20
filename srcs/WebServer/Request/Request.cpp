#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

Request::Request()
    : _rawRequest(""), _server(nullptr), _location(nullptr), _proxyInfo(nullptr)
{
}

Request::Request(const std::string& rawRequest, const std::vector<Server>& servers, const std::unordered_map<std::string, addrinfo*>& proxyInfoMap)
    : _rawRequest(rawRequest), _server(nullptr), _location(nullptr), _proxyInfo(nullptr)
{
    try
    {
        parseRequest();
        RequestValidator(*this, servers, proxyInfoMap).validate();
        if (!_server || !_location)
            throw std::runtime_error("No matching Server or location found for request");
    }
    catch (const std::exception& e)
    {
        throw ;
    }
}

std::string Request::extractUri(const std::string& requestLine) const
{
    std::string uri = requestLine.substr(requestLine.find(" ") + 1);
    uri = uri.substr(0, uri.find(" "));
    return uri;
}

void Request::extractHeaders()
{
    std::istringstream stream(_rawRequest);
    std::string line;
    while (std::getline(stream, line) && line != "\r")
    {
        size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            _requestData.headers[key] = value;
        }
    }
    _requestData.content_type = _requestData.headers["Content-Type"];
    if (_requestData.headers.find("Content-Length") != _requestData.headers.end())
        _requestData.content_length = std::stoi(_requestData.headers["Content-Length"]);
}

void Request::extractBody()
{
    size_t bodyPos = _rawRequest.find("\r\n\r\n");
    if (bodyPos != std::string::npos)
        _requestData.body = _rawRequest.substr(bodyPos + 4);
}

void Request::parseRequest()
{
    std::istringstream stream(_rawRequest);
    std::string requestLine;
    std::getline(stream, requestLine);
    std::string uri = extractUri(requestLine);

    _requestData.method = requestLine.substr(0, requestLine.find(' '));
    _requestData.uri = uri;

    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos)
    {
        _requestData.query_string = uri.substr(queryPos + 1);
        _requestData.uri = uri.substr(0, queryPos);
    }
    extractHeaders();
    extractBody();
}

const std::string& Request::getRawRequest() const { return _rawRequest; }

const RequestData& Request::getRequestData() const { return _requestData; }

const Server* Request::getServer() const { return _server; }

const Location* Request::getLocation() const { return _location; }

addrinfo* Request::getProxyInfo() const { return _proxyInfo; }
