#include "Request.hpp"
#include "WebServer.hpp"
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
            throw std::runtime_error( "Error validating request" );
    }
    catch (const std::exception& e)
    {
        throw ;
    }
}

void Request::parseRequest(void)
{
    try
    {
        std::istringstream stream(_rawRequest);
        std::string requestLine;
        std::getline(stream, requestLine);

        parseRequestLine(requestLine);
        parseHeadersAndBody(stream);
        parseCookies();
        setContentTypeAndLength();
    }
    catch (const std::exception& e)
    {
        throw ;
    }
}

void Request::parseRequestLine(const std::string& requestLine)
{
    try
    {
        std::string::size_type methodEnd = requestLine.find(' ');
        if (methodEnd == std::string::npos)
            throw std::runtime_error( "Invalid request line: missing method" );

        std::string::size_type uriEnd = requestLine.find(' ', methodEnd + 1);
        if (uriEnd == std::string::npos)
            throw std::runtime_error( "Invalid request line: missing URI" );

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
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string( "Error parsing request line: ") + e.what() );
    }
}

void Request::parseHeadersAndBody(std::istringstream& stream)
{
    _totalHeaderSize = 0;
    try
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
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string( "Error parsing headers and body: ") + e.what() );
    }
}

void Request::parseHeaderLine(const std::string& line)
{
    try
    {
        size_t pos = line.find(':');
        if (pos == std::string::npos)
            throw std::runtime_error( "Invalid header line: missing ':'" );
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        _totalHeaderSize += key.length();
        _totalHeaderSize += value.length();
        _requestData.headers[key] = value;
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string( "Error parsing header line: ") + e.what() );
    }
}

void Request::parseCookies()
{
    try
    {
        auto it = _requestData.headers.find("Cookie");
        if (it != _requestData.headers.end())
        {
            std::string         cookieHeader = it->second;
            std::istringstream  cookieStream(cookieHeader);
            std::string         cookiePair;

            while (std::getline(cookieStream, cookiePair, ';'))
            {
                size_t eqPos = cookiePair.find('=');
                if (eqPos != std::string::npos)
                {
                    std::string key = WebParser::trimSpaces(cookiePair.substr(0, eqPos));
                    std::string value = WebParser::trimSpaces(cookiePair.substr(eqPos + 1));
                    _requestData.cookies[key] = value;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string( "Error parsing cookies: ") + e.what() );
    }
}

void Request::setContentTypeAndLength()
{
    try
    {
        _requestData.content_type = _requestData.headers["Content-Type"];
        if (_requestData.headers.find("Content-Length") != _requestData.headers.end())
        {
            _requestData.content_length = _requestData.headers["Content-Length"];
        }
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string( "Error setting content type and length: ") + e.what() );
    }
}

const std::string&  Request::getRawRequest() const { return _rawRequest; }

const RequestData&  Request::getRequestData() const { return _requestData; }

const Server*       Request::getServer() const { return _server; }

const Location*     Request::getLocation() const { return _location; }

addrinfo*           Request::getProxyInfo() const { return _proxyInfo; }

int                 Request::getErrorCode() const { return _errorCode; }
