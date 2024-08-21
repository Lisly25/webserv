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
        std::cout << "Request validated" << _requestData << std::endl;
    }
    catch (const std::exception& e)
    {
        throw ;
    }
}

void Request::parseRequest(void)
{
    std::istringstream  stream(_rawRequest);
    std::string         requestLine;
    std::getline(stream, requestLine);

    parseRequestLine(requestLine);
    parseHeadersAndBody(stream);
    setContentTypeAndLength();
}

void Request::parseRequestLine(const std::string& requestLine)
{
    std::string::size_type methodEnd = requestLine.find(' ');
    std::string::size_type uriEnd = requestLine.find(' ', methodEnd + 1);
    
    _requestData.method = requestLine.substr(0, methodEnd);
    _requestData.uri = requestLine.substr(methodEnd + 1, uriEnd - methodEnd - 1);

    _requestData.httpVersion = WebParser::trimSpaces(requestLine.substr(uriEnd + 1));

    size_t queryPos = _requestData.uri.find('?');
    if (queryPos != std::string::npos)
    {
        _requestData.query_string = _requestData.uri.substr(queryPos + 1);
        _requestData.uri = _requestData.uri.substr(0, queryPos);
    }
}


void Request::parseHeadersAndBody(std::istringstream& stream)
{
    std::string line;
    bool isHeaderSection = true;

    while (std::getline(stream, line))
    {
        if (isHeaderSection)
        {
            if (line == "\r" || line.empty())
            {
                isHeaderSection = false;
                continue;
            }

            parseHeaderLine(line);
        }
        else
        {
            _requestData.body += line + "\n";
        }
    }
}

void Request::parseHeaderLine(const std::string& line)
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

void Request::setContentTypeAndLength()
{
    _requestData.content_type = _requestData.headers["Content-Type"];
    if (_requestData.headers.find("Content-Length") != _requestData.headers.end())
    {
        _requestData.content_length = std::stoi(_requestData.headers["Content-Length"]);
    }
}

const std::string& Request::getRawRequest() const { return _rawRequest; }

const RequestData& Request::getRequestData() const { return _requestData; }

const Server* Request::getServer() const { return _server; }

const Location* Request::getLocation() const { return _location; }

addrinfo* Request::getProxyInfo() const { return _proxyInfo; }
