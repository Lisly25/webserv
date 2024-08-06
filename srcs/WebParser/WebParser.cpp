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

std::vector<Server> WebParser::getServers(void) const
{
    return (_servers);
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

ssize_t  WebParser::locateContextEnd(size_t contextStart) const
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
        {
            contextEnd = contextStart;
            break ;
        }
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
ssize_t WebParser::locateDirective(size_t contextStart, size_t contextEnd, std::string key) const
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
        if (locateServerContextStart(_configFile[i], "server") == true)
        {
            ssize_t  contextEnd = locateContextEnd(i);
            if (contextEnd == -1)
                throw WebErrors::ConfigFormatException("Error: context not closed properly");
            extractServerInfo(i, contextEnd);
            i = contextEnd + 1;        
        }
        else
            i++;
    }
    if (_servers.empty())
        throw WebErrors::ConfigFormatException("Error: configuration file must contain at least one server context");
}

void WebParser::extractServerInfo(size_t contextStart, size_t contextEnd)
{
    Server  currentServer;
    _servers.push_back(currentServer);

    _servers.back().port = extractPort(contextStart, contextEnd);
    _servers.back().server_name = extractServerName(contextStart, contextEnd);
    _servers.back().client_max_body_size = extractClientMaxBodySize(contextStart, contextEnd);
    extractErrorPageInfo(contextStart, contextEnd);
    //host info still needs to be extracted

    size_t i;
    i = contextStart + 1;
    while (i < contextEnd)
    {
        if (locateLocationContextStart(_configFile[i], "location") == true)
        {
            ssize_t  contextEnd = locateContextEnd(i);
            if (contextEnd == -1)
                throw WebErrors::ConfigFormatException("Error: context not closed properly");
            extractLocationInfo(i, contextEnd);
            i = contextEnd + 1;        
        }
        else
            i++;
    }
}

void    WebParser::extractLocationInfo(size_t contextStart, size_t contextEnd)
{
    Location    currentLocation;

    currentLocation.allowedDELETE = false;
    currentLocation.allowedGET = false;
    currentLocation.allowedPOST = false;
    currentLocation.uri = extractLocationUri(contextStart);
    _servers.back().locations.push_back(currentLocation);
    extractAllowedMethods(contextStart, contextEnd);
}

int WebParser::extractPort(size_t contextStart, size_t contextEnd) const
{
    std::string key = "listen";
    ssize_t directiveLocation = locateDirective(contextStart, contextEnd, key);
   
    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: multiple listening ports per server");
    if (directiveLocation == 0)
        throw WebErrors::ConfigFormatException("Error: missing listening port");
    
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);

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
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);

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
            throw WebErrors::BaseException("Vector operaation failed");
        }
    }   
    return (serverNames);
}

//0 will be returned if no limit was set in the file
//otherwise, will return the value in bytes
long WebParser::extractClientMaxBodySize(size_t contextStart, size_t contextEnd) const
{
    std::string key = "client_max_body_size";
    ssize_t    directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: can only have one client_max_body_size directive");
    if (directiveLocation == 0)
        return (1000000);//nginx's default is 1M
    
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
        
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
//Will consider the field optional for now
void    WebParser::extractErrorPageInfo(size_t contextStart, size_t contextEnd)
{
    std::string key = "error_page";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: can only have one error_page directive");
    if (directiveLocation == 0)
        return ;

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    
    size_t  i;
    i = line.length();
    while (i > 0 && !isspace(line[i]))
        i--;
    if (i == line.length())
        throw WebErrors::ConfigFormatException("Error: must specify page address for error_page directive");
    std::string errorAddress = line.substr(i, line.length() - i);
    if (errorAddress.find('/') == std::string::npos)
        throw WebErrors::ConfigFormatException("Error: address of error page should be an URI/URL (but the given one did not even contain '/')");
    _servers.back().error_page = errorAddress;
    line = line.substr(0, i);
    i = 0;
    while (line[i])
    {
        if (!isspace(line[i]) && (line[i] > '9' || line[i] < '0'))
            throw WebErrors::ConfigFormatException("Error: default error page must be expressed in one URI/URL");
        i++;
    }

    std::stringstream stream(line);
    int         errorCode;
    std::vector<int>    errorCodeVec;

    while (1)
    {
        stream >> errorCode;
        if (stream.fail())
            break ;
        if (errorCode < 400 || errorCode > 599)
            throw WebErrors::ConfigFormatException("Error: default error page was specified for invalid error code. Valid range is 400 - 599");
        try
        {
            errorCodeVec.push_back(errorCode);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            throw WebErrors::BaseException("Vector operation failed");
        }
    }
    if (errorCodeVec.empty())
        throw WebErrors::ConfigFormatException("Error: must specify error codes within error_page directive");
    stream >> errorAddress;
    _servers.back().error_codes = errorCodeVec;
}

void WebParser::printParsedInfo(void)
{
    size_t i = 0;
    std::vector<Server> servers = this->_servers;

    while (i < servers.size())
    {
        std::cout << "Server nro." << i << std::endl;
        std::cout << "Listening on port: " << servers[i].port << std::endl;
        std::cout << "Server names: " << std::endl;
        for (size_t j = 0; j < servers[i].server_name.size(); j++)
        {
            std::cout << servers[i].server_name[j] << std::endl;
        }
        std::cout << "Default error page address: " << servers[i].error_page << std::endl;
        std::cout << "And the error codes it is applied to: " << std::endl;
        for (size_t k = 0; k < servers[i].error_codes.size(); k++)
        {
            std::cout << servers[i].error_codes[k] << std::endl;
        }
        std::cout << "Client body max size in bytes: " << servers[i].client_max_body_size << std::endl;
        std::cout << "Location info for this server: " << std::endl;
        for (size_t h = 0; h < servers[i].locations.size(); h++)
        {
            std::cout << ">>> Location nro." << h << std::endl;
            std::cout << ">>> URI: " << servers[i].locations[h].uri << std::endl;
            std::cout << ">>> Allowed HTTP methods: " << std::endl << "GET: ";
            if (servers[i].locations[h].allowedGET == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << "POST: ";
            if (servers[i].locations[h].allowedPOST == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << "DELETE: ";
            if (servers[i].locations[h].allowedDELETE == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
        }
        std::cout << std::endl;
        i++;
    }
}

std::string WebParser::extractLocationUri(size_t contextStart) const
{
    std::string key = "location";

    std::string line = _configFile[contextStart];
    size_t startIndex = line.find(key) + key.length();
    while (isspace(line[startIndex]))
        startIndex++;
    line = line.substr(startIndex, line.length() - startIndex - 2);
    return (line);
}

void    WebParser::extractAllowedMethods(size_t contextStart, size_t contextEnd)
{
    std::string key = "allowed_methods";
    ssize_t directiveLocation = locateDirective(contextStart, contextEnd, key);
    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one allowed_methods per location context is allowed");
    if (directiveLocation == 0)
        throw WebErrors::ConfigFormatException("Error: please add the allowed_methods directive to all location contexts");

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);

    std::string subLine;
    std::istringstream  stream(line);

    while (getline(stream, subLine, ' '))
    {
        if (subLine.compare("GET") == 0)
        {
            if (_servers.back().locations.back().allowedGET == true)
                throw WebErrors::ConfigFormatException("Error: GET is listed twice in the same allowed_methods directive");
            _servers.back().locations.back().allowedGET = true;
        }
        else if (subLine.compare("POST") == 0)
        {
            if (_servers.back().locations.back().allowedPOST == true)
                throw WebErrors::ConfigFormatException("Error: POST is listed twice in the same allowed_methods directive");
            _servers.back().locations.back().allowedPOST = true;
        }
        else if (subLine.compare("DELETE") == 0)
        {
            if (_servers.back().locations.back().allowedDELETE == true)
                throw WebErrors::ConfigFormatException("Error: DELETE is listed twice in the same allowed_methods directive");
            _servers.back().locations.back().allowedDELETE = true;
        }
        else
            throw WebErrors::ConfigFormatException("Error: allowed_methods directive accepts only 3 values: GET, POST, DELETE");
    }   
}