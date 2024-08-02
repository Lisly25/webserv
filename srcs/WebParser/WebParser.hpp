#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <stack>
#include <vector>
#include <sstream>

enum LocationType { CGI, PROXY };

struct Location {
    LocationType    type;
    std::string     path;
    std::string     target;
};

struct Server {
    int                            port;
    std::vector<std::string>       server_name;
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
    bool                        checkSemicolon(std::string line);
    bool                        checkComment(std::string line);
    std::string                 removeInLineComment(std::string line);
    bool                        checkBracePairs(std::string line);
    bool                        checkBracesPerLine(std::string line);
    bool                        locateContextStart(std::string line, std::string contextName);
    ssize_t                     locateContextEnd(size_t contextStart);
    ssize_t                     locateDirective(size_t contextStart, size_t contextEnd, std::string key);
    void                        parseServer(void);
    void                        extractServerInfo(size_t contextStart, size_t contextEnd);
    void                        extractLocationInfo(size_t contextStart);
    int                         extractPort(size_t contextStart, size_t contextEnd);
    std::vector<std::string>    extractServerName(size_t contextStart, size_t contextEnd);
};
