#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <netdb.h> 
#include "WebParser.hpp"

struct RequestData
{
    std::string httpVersion;
    std::string method;
    std::string uri;
    std::string query_string;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> cookies;
    std::string body;  
    std::string script_filename;
    std::string content_type; 
    std::string content_length;
    std::string resolvedPath;
    std::string absoluteRootPath;
    bool        shouldAutoIndex;
};

inline std::ostream& operator<<(std::ostream& os, const RequestData& requestData)
{
    os << "Method: " << requestData.method << "\n";
    os << "URI: " << requestData.uri << "\n";
    os << "Query String: " << requestData.query_string << "\n";
    os << "Version: " << requestData.httpVersion << "\n";  // Added output for HTTP version
    os << "Headers:\n";
    for (const auto& header : requestData.headers) {
        os << "  " << header.first << ": " << header.second << "\n";
    }
    os << "Body: " << requestData.body << "\n";
    os << "Script Filename: " << requestData.script_filename << "\n";
    os << "Content Type: " << requestData.content_type << "\n";
    os << "Content Length: " << requestData.content_length << "\n";
    return os;
}

enum ErrorCodes { INVALID_METHOD = 405, NOT_FOUND = 404, HTTP_VERSION_NOT_SUPPORTED = 505,\
    BAD_REQUEST = 400, REQUEST_BODY_TOO_LARGE = 413, FORBIDDEN = 403, };

class Request
{
public:
    Request();
    Request(const std::string& rawRequest, const std::vector<Server>& servers,\
        const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);

    const std::string&  getRawRequest() const;
    const Server*       getServer() const;
    const Location*     getLocation() const;
    addrinfo*           getProxyInfo() const;
    const RequestData&  getRequestData() const;
    int                 getErrorCode() const;
private:
    RequestData     _requestData = {};
    std::string     _rawRequest;
    const Server*   _server = nullptr;
    const Location* _location = nullptr;
    addrinfo*       _proxyInfo;

    int             _errorCode = 0;

    void        parseCookies();  
    void        parseRequest(void);
    void        parseHeadersAndBody(std::istringstream& stream);
    void        parseHeaderLine(const std::string& line);
    void        setContentTypeAndLength();
    std::string extractUri(const std::string& rawRequest) const;
    void        parseRequestLine(const std::string& requestLine);


public:
    class RequestValidator
    {
    public:
        RequestValidator(Request& request, const std::vector<Server>& servers,\
            const std::unordered_map<std::string, addrinfo*>& proxyInfoMap);
        ~RequestValidator() = default;
        bool validate() const;

    private:
        Request&                                            _request;
        const std::vector<Server>&                          _servers;
        const std::unordered_map<std::string, addrinfo*>&   _proxyInfoMap;

        bool checkForIndexing(std::string& fullPath) const;
        bool isPathValid()      const;
        bool isReadOk()      const;
        bool isValidMethod()    const;
        bool isProtocolValid()  const;
        bool isMethodValid()    const;
        bool areHeadersValid()  const;
        bool isServerMatch(const Server& server) const;
        bool matchLocationSetData(const Server& server) const;
    };
};

