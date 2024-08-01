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

//should also add a checker against multiple braces on a line
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
            return (contextStart);
        contextStart++;
    }
    return (-1);
}

void WebParser::parseServer(void)
{
    size_t i;

    i = 0;
    while (i < _configFile.size())
    {
        if (locateContextStart(_configFile[i], "server") == true)
        {
            extractServerInfo(i);
            i = 0;        
        }
        i++;
    }
    //Here we need to check if the _servers vector is empty, throw an exception if it is
}

void WebParser::extractServerInfo(size_t contextStart)
{
    ssize_t  contextEnd = locateContextEnd(contextStart);
    if (contextEnd == -1)
        throw WebErrors::ConfigFormatException("Error: context not closed properly");
    std::cout << "Server context ends at line " << contextEnd << std::endl;

}