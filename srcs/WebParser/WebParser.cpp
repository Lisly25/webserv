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
        line = removeInLineComment(line);
        if (!checkBraces(line))
            throw WebErrors::ConfigFormatException("Error: unclosed braces");
        if (!checkSemicolon(line))
            throw WebErrors::ConfigFormatException("Error: directives in config files must be followed by semicolons");
        try
        {
            _configFile.push_back(line);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
        //if (line.find("server") != std::string::npos)
        //    parseServer(line);
        if (line.find("proxy_pass") != std::string::npos)
            parseProxyPass(line);
        if (line.find("cgi_pass") != std::string::npos)
            parseCgiPass(line);
    }
    if (!_bracePairCheckStack.empty())
        throw WebErrors::ConfigFormatException("Error: unclosed braces");
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

std::string WebParser::removeInLineComment(std::string line)
{
    size_t hashtagLocation;

    hashtagLocation = line.find_first_of('#');
    if (hashtagLocation == std::string::npos)
        return (line);
    line = line.substr(0, hashtagLocation);
    std::cout << "Line after comment removal: " << line << std::endl;
    return (line);
}

bool WebParser::checkBraces(std::string line)
{
    int i = 0;
   
    while (line[i])
    {
        if (line[i] == '{')
            _bracePairCheckStack.push(line[i]);
        else if (line[i] == '}')
        {
            if (_bracePairCheckStack.empty())
                return (false);
            if (_bracePairCheckStack.top() == '{')
                _bracePairCheckStack.pop();
            else
                return (false);
        }
        i++;
    }
    return (true);
}

//this is meant to check that a line contains just a keyword, and no other substrings)
// - if isContext is true,it will expect a '{' at the end of the string
// - if isContext is false, it will just verify that the string starts with keyword, and is followed by a whitespace char
/*bool WebParser::verifyKeyword(std::string line, std::string keyword, bool isContext)
{
    int keyword_start = line.find(keyword);
    int i = 0;
    while (i < keyword_start)
    {
        if (!isspace(line[i]))
            return (false);
        i++;
    }
    i += keyword.length();
    while (i < line.length() && isspace(line[i]))
    {
        if (isContext)
            return (true);
        i++;
    }
    if (!isContext)
        return (false);
}*/

/*void WebParser::parseServer(std::string line)
{
    
}*/