#pragma once

#include <iostream>
#include <fstream>
#include <string>

class ConfigParser
{
public:
    ConfigParser(const std::string &filename);
    ~ConfigParser() = default;

    ConfigParser(const ConfigParser &) = delete;
    ConfigParser &operator=(const ConfigParser &) = delete;
    
    bool        parse();
    std::string getProxyPass() const;
    std::string getCgiPass() const;

private:
    std::string     _filename;
    std::ifstream   _file;
    std::string     _proxyPass;
    std::string     _cgiPass;

    void parseProxyPass(const std::string &line);
    void parseCgiPass(const std::string &line);
};
