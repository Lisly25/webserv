#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <stack>

class WebParser
{
public:
    WebParser(const std::string &filename);
    ~WebParser() = default;

    WebParser(const WebParser &) = delete;
    WebParser &operator=(const WebParser &) = delete;
    
    bool        parse();
    std::string getProxyPass() const;
    std::string getCgiPass() const;

private:
    std::string         _filename;
    std::ifstream       _file;
    std::string         _proxyPass;
    std::string         _cgiPass;
    std::stack<char>    _bracePairCheckStack;

    void parseProxyPass(const std::string &line);
    void parseCgiPass(const std::string &line);
    bool checkSemicolon(std::string line);
    bool checkComment(std::string line);
    bool checkBraces(std::string line);
    bool checkFormat(void);
};
