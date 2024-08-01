#include "WebParser.hpp"
#include "WebErrors.hpp"

WebParser::WebParser(const std::string &filename) 
:  _filename(filename), _file(filename)
{
    std::filesystem::path filePath = filename;
    if (filePath.extension() != ".conf")//.extension is a function from c++17
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
        if (!checkBracePairs(line))
            throw WebErrors::ConfigFormatException("Error: unclosed braces");
        if (!checkBracesPerLine(line))
            throw WebErrors::ConfigFormatException("Error: multiple braces on one line");
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
        if (line.find("proxy_pass") != std::string::npos)
            parseProxyPass(line);
        if (line.find("cgi_pass") != std::string::npos)
            parseCgiPass(line);
    }
    if (!_bracePairCheckStack.empty())
        throw WebErrors::ConfigFormatException("Error: unclosed braces");
    _file.close();
    parseServer();
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
    return (line);
}

bool WebParser::checkBracePairs(std::string line)
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

bool WebParser::checkBracesPerLine(std::string line)
{
    int braceCount = 0;
    int i = 0;

    while (line[i])
    {
        if (line[i] == '{' || line[i] == '}')
            braceCount++;
        i++;
    }
    if (braceCount <= 1)
        return (true);
    return (false);
}

/*
The context format we expect is:
server {
    ...
}
whitespaces can be added anywhere, and at least one whitespace is expected between the context name and the opening brace
*/

bool    WebParser::locateContextStart(std::string line, std::string contextName)
{
    size_t keywordStart = line.find(contextName);

    if (keywordStart == std::string::npos)
        return (false);

    size_t i = 0;

    while (i < keywordStart)
    {
        if (!isspace(line[i]))
            return (false);
        i++;
    }

    size_t j;

    i += contextName.length();
    j = i;

    while (isspace(line[i]))
        i++;
    if (i == j)
        return (false);
    if (line[i] != '{')
        return (false);
    if (i < line.length())
        i++;
    while (line[i])
    {
        if (!isspace(line[i]))
            return (false);
        i++;
    }
    return (true);
}

ssize_t  WebParser::locateContextEnd(size_t contextStart)
{
    int leftBraceCount = 1;
    int rightBraceCount = 0;
    ssize_t contextEnd = -1;

    contextStart++;
    while (contextStart < _configFile.size())
    {
        for (size_t i = 0; i < _configFile[contextStart].length(); i++)
        {
            if (_configFile[contextStart][i] == '{')
                leftBraceCount++;
            else if (_configFile[contextStart][i] == '}')
                rightBraceCount++;
        }
        if (leftBraceCount == rightBraceCount)
            contextEnd = contextStart;
        contextStart++;
    }
    if (contextEnd == -1)
        return (-1);
    //verifying that the line containing the closing brace contains only whitespaces besides the brace - we already know there is only one brace, and it is a closing one
    for (size_t i = 0; i < _configFile[contextEnd].length(); i++)
    {
        if (!isspace(_configFile[contextEnd][i]) && _configFile[contextEnd][i] != '}')
            return (-1);
    }
    return (contextEnd);
}

void WebParser::parseServer(void)
{
    size_t i;

    i = 0;
    while (i < _configFile.size())
    {
        if (locateContextStart(_configFile[i], "server") == true)
        {
            ssize_t  contextEnd = locateContextEnd(i);
            if (contextEnd == -1)
                throw WebErrors::ConfigFormatException("Error: context not closed properly");
            extractServerInfo(i, contextEnd);
            i = 0;        
        }
        i++;
    }
    //Here we need to check if the _servers vector is empty, throw an exception if it is
}

void WebParser::extractServerInfo(size_t contextStart, size_t contextEnd)
{
    std::cout << "Server context starts at " << contextStart << " and ends at line " << contextEnd << std::endl;
}