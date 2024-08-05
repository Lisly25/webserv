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
            throw WebErrors::BaseException("Failed to read config file into vector");//Not sure if this is necessary
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

    if (i == 0)
        return (true);
    i--;
    if (line[i] == ';' || line[i] == '}' || line[i] == '{')
        return (true);
    return (false);
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
whitespaces can be added anywhere (except at the ends of lines), and at least one whitespace is expected between the context name and the opening brace
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

//should return -1 if there's several in the given range, otherwise the index within the vector
//if it can't be found, return 0 (it would never be on line 0, since a valid file would at best have the server directive there)
ssize_t WebParser::locateDirective(size_t contextStart, size_t contextEnd, std::string key)
{
    size_t  i;
    size_t  key_start;
    ssize_t directive_index;
    int     matches;

    i = 0;
    matches = 0;
    while (contextStart < contextEnd)
    {
        while (isspace(_configFile[contextStart][i]))
            i++;
        key_start = _configFile[contextStart].find(key, i);
        if (key_start == i)
        {
            matches++;
            directive_index = contextStart;
        }
        contextStart++;
        i = 0;
    }
    switch (matches)
    {
    case 0:
        return (0);
    case 1:
        return (directive_index);
    default:
        return (-1);
    }
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
    Server  currentServer;

    currentServer.port = extractPort(contextStart, contextEnd);
    currentServer.server_name = extractServerName(contextStart, contextEnd);
    currentServer.client_max_body_size = extractClientMaxBodySize(contextStart, contextEnd);
    std::cout << "max body size is: " << currentServer.client_max_body_size << std::endl;
}

int WebParser::extractPort(size_t contextStart, size_t contextEnd)
{
    std::string key = "listen";
    ssize_t directiveLocation = locateDirective(contextStart, contextEnd, key);
   
    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: multiple listening ports per server");
    if (directiveLocation == 0)
        throw WebErrors::ConfigFormatException("Error: missing listening port");
    
    std::string line = _configFile[directiveLocation];
    size_t portIndex = line.find(key) + key.length();
    while (isspace(line[portIndex]))
        portIndex++;
    line = line.substr(portIndex, line.length() - portIndex - 1);

    std::stringstream stream(line);
    int               portNumber;
    std::string       userInput;

    stream >> portNumber;
    if (stream.fail())
        throw WebErrors::ConfigFormatException("Error: listening port specified is not a number");
    std::string leftover;
    stream >> leftover;
    if (!leftover.empty())
        throw WebErrors::ConfigFormatException("Error: listening port specified is not (just) a number");
    if (portNumber < 0 || portNumber > 65535)
        throw WebErrors::ConfigFormatException("Error: invalid number for port");
    if (portNumber <= 1023)
    {
        std::cout << "Specified listening port is under 1023 (you need sudo permission to use these) are you sure you want to continue? (Y/N)" << std::endl;
        while (1)
        {
            std::cin >> userInput;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (userInput.empty() || !userInput.compare("N"))
            {
                std::cout << "Exiting..." << std::endl;
                throw WebErrors::ConfigFormatException("Error: port number under 1024");//This might be weird, but just exiting caused a memory leak
            }
            if (!userInput.compare("Y"))
                break ;
        }
    }
    return (portNumber);
}

std::vector<std::string> WebParser::extractServerName(size_t contextStart, size_t contextEnd)
{
    std::string                 key = "server_name";
    ssize_t                     directiveLocation = locateDirective(contextStart, contextEnd, key);
    std::vector<std::string>    serverNames;

    switch (directiveLocation)
    {
    case 0:
        return (serverNames);
    case -1:
        throw WebErrors::ConfigFormatException("Error: multiple server_name fields within context. If you want to specify several server names, the valid format is: 'server_name <name1> <name2> ... <name_n>'");
    default:
        break;
    }
    std::string line = _configFile[directiveLocation];
    size_t snameIndex = line.find(key) + key.length();

    while (isspace(line[snameIndex]))
        snameIndex++;
    line = line.substr(snameIndex, line.length() - snameIndex - 1);

    std::string subLine;
    std::istringstream  stream(line);

    while (getline(stream, subLine, ' '))
    {
        try
        {
            serverNames.push_back(subLine);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            throw WebErrors::BaseException("Failed to read config file into vector");//Not sure if this is necessary
        }
    }   
    return (serverNames);
}

//0 will be returned if no limit was set in the file
//otherwise, will return the value in bytes
long WebParser::extractClientMaxBodySize(size_t contextStart, size_t contextEnd)
{
    std::string key = "client_max_body_size";
    ssize_t    directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: can only have one client_max_body_size directive");
    if (directiveLocation == 0)
        return (1000000);//nginx's default is 1M
    
    std::string line = _configFile[directiveLocation];
    size_t bsizeIndex = line.find(key) + key.length();

    while (isspace(line[bsizeIndex]))
        bsizeIndex++;
    line = line.substr(bsizeIndex, line.length() - bsizeIndex - 1);
        
    std::stringstream stream(line);
    long numericComponent;
    std::string alphabetComponent;

    stream >> numericComponent;
    if (stream.fail() || numericComponent < 0)
        throw WebErrors::ConfigFormatException("Error: client_max_body_size does not have a non-negative numeric component smaller than LONG_MAX");
    stream >> alphabetComponent;
    if (stream.fail() && numericComponent == 0)
        return (0);
    if (stream.fail())
        throw WebErrors::ConfigFormatException("Error: client_max_body_size must have unit specified 'K' for kilobytes, 'M' for megabytes");
    if (alphabetComponent.compare("K") == 0)
    {
        if (numericComponent > (LONG_MAX / 1000))
            throw WebErrors::ConfigFormatException("Error: client_max_body_size can't be larger than LONG_MAX");
        numericComponent *= 1000;
    }
    else if (alphabetComponent.compare("M") == 0)
    {
        if (numericComponent > (LONG_MAX / 1000000))
            throw WebErrors::ConfigFormatException("Error: client_max_body_size can't be larger than LONG_MAX");
        numericComponent *= 1000000;
    }
    else
        throw WebErrors::ConfigFormatException("Error: client_max_body_size must have unit specified 'K' for kilobytes, 'M' for megabytes");
    return (numericComponent);
}

//Extracts both the error codes, and the error page address. Does not process the error address yet
//Will consider it optional for now
void    WebParser::extractErrorPageInfo(size_t contextStart, size_t contextEnd)
{
    std::string key = "error_page";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: can only have one error_page directive");
    if (directiveLocation == 0)
        return ;
}