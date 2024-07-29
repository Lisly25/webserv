#include "ConfigParser.hpp"
#include "WebErrors.hpp"

ConfigParser::ConfigParser(const std::string &filename) 
:  _filename(filename), _file(filename)
{
    if (!_file.is_open())
        throw WebErrors::FileOpenException(_filename);
}

bool ConfigParser::parse()
{
    std::string line;
    while (std::getline(_file, line))
    {
        if (line.find("proxy_pass") != std::string::npos)
            parseProxyPass(line);
        if (line.find("cgi_pass") != std::string::npos)
            parseCgiPass(line);
    }
    _file.close();
    return true;
}

void ConfigParser::parseProxyPass(const std::string &line)
{
    size_t pos = line.find("http://");
    if (pos != std::string::npos)
    {
        _proxyPass = line.substr(pos);
        _proxyPass = _proxyPass.substr(0, _proxyPass.find(';'));
    }
}

void ConfigParser::parseCgiPass(const std::string &line)
{
    size_t pos = line.find("tests/");
    if (pos != std::string::npos)
    {
        _cgiPass = line.substr(pos);
        _cgiPass = _cgiPass.substr(0, _cgiPass.find(';'));
    }
}

std::string ConfigParser::getProxyPass() const { return _proxyPass; }

std::string ConfigParser::getCgiPass() const { return _cgiPass; }
