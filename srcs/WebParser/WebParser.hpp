#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <stack>
#include <vector>
#include <sstream>
#include <climits>
#include <unistd.h>
#include <cstring>

enum LocationType { CGI, PROXY, ALIAS, STANDARD };

struct Location {
    LocationType                type;
    std::string                 uri;
    std::string                 root;
    std::string                 target;
    bool                        allowedGET;
    bool                        allowedPOST;
    bool                        allowedDELETE;
    bool                        autoIndexOn;
    std::vector<std::string>    index;
};

struct Server {
    int                            port;
    long                           client_max_body_size;
    std::string                    host;
    std::vector<std::string>       server_name;
    std::vector<int>               error_codes;
    std::string                    error_page;
    std::vector<Location>          locations;
};

class WebParser
{

public:
    WebParser(const std::string &filename);
    ~WebParser() = default;

    WebParser(const WebParser &) = delete;
    WebParser &operator=(const WebParser &) = delete;
    
    bool                parse();
    std::string         getProxyPass() const;
    std::string         getCgiPass() const;
    std::vector<Server> getServers() const;

    //for testing:
    void                printParsedInfo(void);

private:

    std::vector<std::string> _configFile;
    std::string              _filename;
    std::ifstream            _file;
    std::string             _proxyPass;
    std::string             _cgiPass;
    std::stack<char>        _bracePairCheckStack;
    std::vector<Server>     _servers;

    void                        parseProxyPass(const std::string &line);
    void                        parseCgiPass(const std::string &line);
    bool                        checkBracePairs(std::string line);
    ssize_t                     locateContextEnd(size_t contextStart) const;
    ssize_t                     locateDirective(size_t contextStart, size_t contextEnd, std::string key) const;
    void                        parseServer(void);
    void                        extractServerInfo(size_t contextStart, size_t contextEnd);
    void                        extractLocationInfo(size_t contextStart, size_t contextEnd);
    int                         extractPort(size_t contextStart, size_t contextEnd) const;
    std::vector<std::string>    extractServerName(size_t contextStart, size_t contextEnd);
    long                        extractClientMaxBodySize(size_t contextStart, size_t contextEnd) const;
    std::string                 extractHost(size_t contextStart, size_t contextEnd) const;
    void                        extractErrorPageInfo(size_t contextStart, size_t contextEnd);
    std::string                 extractLocationUri(size_t contextStart) const;
    void                        extractAllowedMethods(size_t contextStart, size_t contextEnd);
    std::string                 extractRoot(size_t contextStart, size_t contextEnd) const;
    void                        extractAutoinex(size_t contextStart, size_t contextEnd);
    void                        extractRedirectionAndTarget(size_t contextStart, size_t contextEnd);
    void                        extractIndex(size_t contextStart, size_t contextEnd);

    //in WebParserUtils

    static bool                 checkSemicolon(std::string line);
    static bool                 checkComment(std::string line);
    static std::string          removeInLineComment(std::string line);
    static bool                 checkBracesPerLine(std::string line);
    static bool                 locateServerContextStart(std::string line, std::string contextName);
    static bool                 locateLocationContextStart(std::string line, std::string contextName);
    static std::string          removeDirectiveKey(std::string line, std::string key);
    static std::string          createStandardTarget(std::string uri, std::string root);
    static bool                 verifyTarget(std::string path);
};
