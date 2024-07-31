#include "WebParser.hpp"
#include "WebErrors.hpp"

WebParser::WebParser(const std::string &filename) 
:  _filename(filename), _file(filename)
{
    std::filesystem::path filePath = filename;
    if (filePath.extension() != ".conf")//.extension is a function from c++17 --> change -std flag in Makefile if we keep this. + add compiler flag -lstdc++fs
        throw WebErrors::ConfigFormatException("Error: configuration file must have .conf extension");
    if (!_file.is_open())
        throw WebErrors::FileOpenException(_filename);
}

bool WebParser::parse()
{
    std::string line;
    while (std::getline(_file, line))
    {
        if (checkComment(line))
            continue;
        if (!checkSemicolon(line))
            throw WebErrors::ConfigFormatException("Error: directives in config files must be followed by semicolons");
        if (line.find("proxy_pass") != std::string::npos)
            parseProxyPass(line);
        if (line.find("cgi_pass") != std::string::npos)
            parseCgiPass(line);
    }
    _file.close();
    return true;
}

void WebParser::parseProxyPass(const std::string &line)
{
    size_t pos = line.find("http://");
    if (pos != std::string::npos)
    {
        _proxyPass = line.substr(pos);
        _proxyPass = _proxyPass.substr(0, _proxyPass.find(';'));
    }
}

void WebParser::parseCgiPass(const std::string &line)
{
    size_t pos = line.find("tests/");
    if (pos != std::string::npos)
    {
        _cgiPass = line.substr(pos);
        _cgiPass = _cgiPass.substr(0, _cgiPass.find(';'));
    }
}

std::string WebParser::getProxyPass() const { return _proxyPass; }

std::string WebParser::getCgiPass() const { return _cgiPass; }

/* checkFormat will verify basic requirements:
- file extension
- are directives always followed by semicolons
- are {}s always closed
- ... */

bool WebParser::checkSemicolon(std::string line)
{
    int i = line.length();

    while (isspace(line[i]) && i > 0)
        i--;
    if (i == 0)
        return (true);
    i--;
    if (isalnum(line[i]) && (line[i] != ';' && line[i] != '{' && line[i] != '}'))
        return (false);
    return (true);
}

bool WebParser::checkComment(std::string line)
{
    int i = 0;
    int lineLength = line.length();

    while (i < lineLength && isspace(line[i]))
        i++;
    if (line[i] == '#')
        return (true);
    return (false);
}

bool WebParser::checkFormat(void)
{
    return (true);
}
