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
            WebErrors::printerror("WebParser::parse", e.what());
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

const std::vector<Server> &WebParser::getServers(void) const
{
    return (_servers);
}

//Can we remove this + parseCGIPass now?
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

const std::string &WebParser::getProxyPass() const { return _proxyPass; }

const std::string &WebParser::getCgiPass() const { return _cgiPass; }

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
    size_t configSize;

    i = 0;
    configSize = _configFile.size();
    while (i < configSize)
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
    _servers.back().host = extractHost(contextStart, contextEnd);
    _servers.back().server_root = extractServerRoot(contextStart, contextEnd);
    _servers.back().client_timeout = extractClientTimeout(contextStart, contextEnd);
    extractErrorPageInfo(contextStart, contextEnd);

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
    currentLocation.allowedHEAD = false;
    currentLocation.allowedPOST = false;
    currentLocation.autoIndexOn = false;
    currentLocation.uri = extractLocationUri(contextStart);
    currentLocation.root = extractRoot(contextStart, contextEnd);
    currentLocation.upload_folder = extractUploadFolder(contextStart, contextEnd);
    _servers.back().locations.push_back(currentLocation);
    extractAllowedMethods(contextStart, contextEnd);
    extractAutoinex(contextStart, contextEnd);
    extractRedirectionAndTarget(contextStart, contextEnd);
    extractIndex(contextStart, contextEnd);
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

//optional directive - allowed to be empty (`server_names;`)
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
            WebErrors::printerror("WebParser::extractServerName", e.what());
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

size_t WebParser::extractClientTimeout(size_t contextStart, size_t contextEnd)
{
    std::string key = "client_timeout";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: multiple client_timeout directives per server");
    if (directiveLocation == 0)
        throw WebErrors::ConfigFormatException("Error: missing client_timeout directive for some server block");

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);

    if (line.empty())
        throw WebErrors::ConfigFormatException("Error: client_timeout directive value is missing");
    line = WebParser::trimSpaces(line);
    size_t timeout;
    try
    {
        size_t pos = 0;
        timeout = std::stoul(line, &pos);
        if (pos != line.length())
            throw std::invalid_argument("Extra characters after number");
    }
    catch (const std::exception& e)
    {
        throw WebErrors::ConfigFormatException("Error: invalid client_timeout value");
    }

    return timeout;
}

std::string     WebParser::extractServerRoot(size_t contextStart, size_t contextEnd) const
{
    std::string key = "server_root";
    ssize_t directiveLocation = locateDirective(contextStart, contextEnd, key);
   
    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: multiple server_root directives per server");
    if (directiveLocation == 0)
        return ("");
    
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    if (line.size() == 0)
        return ("");
    if (line[0] != '/')
        throw WebErrors::ConfigFormatException("Error: server_root directive must be an absolute path");
    if (line.back() != '/')
        throw WebErrors::ConfigFormatException("Error: server_root directive must end with '/'");
    if (!verifyTarget(line))
        throw WebErrors::ConfigFormatException("Error: location defined as server_root (" + line + ") does not exist");
    return (line);
}


//optional field, if not set, will set it to 127.0.0.1
//should we also test this by pinging the address if it's not 127.0.0.1? (And throw an error if we didn't successfully ping ourselves)
//if we don't do that, I'll implement further error checks to see if the input corresponds to IP-address format
std::string     WebParser::extractHost(size_t contextStart, size_t contextEnd) const
{
    std::string key = "host";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: can only have one 'host' directive per server context");
    if (directiveLocation == 0)
        return ("127.0.0.1");

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    if (line.size() == 0)
        return ("127.0.0.1");
    return (line);
}

//Extracts both the error codes, and the error page address. Does not process the error address yet
//Will consider the field optional for now
void    WebParser::extractErrorPageInfo(size_t contextStart, size_t contextEnd)
{
    std::string                 keyword = "error_page";
    size_t                      searchStart = contextStart;
    size_t                      searchEnd = contextStart + 1;
    ssize_t                     directiveLocation;
    std::string                 line;
    size_t                      i;
    std::map<int, std::string>  errorPages;
    std::string                 value;
    int                         key;
    
    while (searchEnd <= contextEnd)
    {
        directiveLocation = locateDirective(searchStart, searchEnd, keyword);
        searchEnd++;
        searchStart++;
        if (directiveLocation == 0)
            continue;
        line = removeDirectiveKey(_configFile[directiveLocation], keyword);
        i = line.length();
        while (i > 0 && !isspace(line[i]))
            i--;
        if (i == line.length())
            throw WebErrors::ConfigFormatException("Error: must specify page address for error_page directive");
        value = line.substr(i, line.length() - i);
        value = std::regex_replace(value, std::regex("^ +"), "");
        value = createStandardTarget(value, _servers.back().server_root);
        line = line.substr(0, i);
        i = 0;
        while (line[i])
        {
            if (!isspace(line[i]) && (line[i] > '9' || line[i] < '0'))
                throw WebErrors::ConfigFormatException("Error: default error page must be expressed in one URI/URL");
            i++;
        }
        key = getErrorCode(line);
        errorPages.insert(std::pair<int, std::string>(key, value));
    }
    _servers.back().error_page = errorPages;
}

void WebParser::printParsedInfo(void)
{
    size_t i = 0;
    std::vector<Server> servers = this->_servers;

    while (i < servers.size())
    {
        std::cout << "Server nro." << i << std::endl;
        std::cout << "Host: " << servers[i].host << std::endl;
        std::cout << "Listening on port: " << servers[i].port << std::endl;
        std::cout << "Server names: " << std::endl;
        for (size_t j = 0; j < servers[i].server_name.size(); j++)
        {
            std::cout << servers[i].server_name[j] << std::endl;
        }
        std::cout << "Server-wide root: " << servers[i].server_root << std::endl;
        std::cout << "Map of error codes and pages: " << std::endl;
        for (auto const &pair: servers[i].error_page)
        {
            std::cout << "Code: " << pair.first << " - Page: " << pair.second << std::endl;
        }
        std::cout << "Client body max size in bytes: " << servers[i].client_max_body_size << std::endl;
        std::cout << "Location info for this server: " << std::endl;
        for (size_t h = 0; h < servers[i].locations.size(); h++)
        {
            std::cout << ">>> Location nro." << h << std::endl;
            std::cout << ">>> URI: " << servers[i].locations[h].uri << std::endl;
            std::cout << ">>> Allowed HTTP methods: " << std::endl << ">>> GET: ";
            if (servers[i].locations[h].allowedGET == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << ">>> POST: ";
            if (servers[i].locations[h].allowedPOST == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << ">>> DELETE: ";
            if (servers[i].locations[h].allowedDELETE == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << ">>> HEAD: ";
            if (servers[i].locations[h].allowedHEAD == true)
                std::cout << "yes" << std::endl;
            else
                std::cout << "no" << std::endl;
            std::cout << ">>> root directory: " << servers[i].locations[h].root << std::endl;
            std::cout << ">>> autoindexing: ";
            if (servers[i].locations[h].autoIndexOn == true)
                std::cout << "on" << std::endl;
            else
                std::cout << "off" << std::endl;
            std::cout << ">>> Redirection type {HTTP, CGI, PROXY, ALIAS, STANDARD}: " << servers[i].locations[h].type << std::endl;
            std::cout << ">>> Target: " << servers[i].locations[h].target << std::endl;
            std::cout << ">>> Index files:" << std::endl;
            for (size_t s = 0; s < servers[i].locations[h].index.size(); s++)
            {
                std::cout << ">>>   " << servers[i].locations[h].index[s] << std::endl;
            }
            std::cout << "Upload folder: " << servers[i].locations[h].upload_folder << std::endl;
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
    if (line.length() == 0)
        throw WebErrors::ConfigFormatException("Error: allowed_methods directive must have a value");

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
        else if (subLine.compare("HEAD") == 0)
        {
            if (_servers.back().locations.back().allowedHEAD == true)
                throw WebErrors::ConfigFormatException("Error: HEAD is listed twice in the same allowed_methods directive");
            _servers.back().locations.back().allowedHEAD = true;
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
            throw WebErrors::ConfigFormatException("Error: allowed_methods directive accepts only 4 values: GET, POST, DELETE, HEAD");
    }   
}

std::string             WebParser::extractUploadFolder(size_t contextStart, size_t contextEnd)
{
    std::string key = "upload_folder";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one 'upload_folder' directive per location context is allowed");
    
    if (directiveLocation == 0)
        return ("uploads");

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    if (line.length() == 0)
        throw WebErrors::ConfigFormatException("Error: upload_folder directive must have a value");        
    return (line);
}

//location context should always contain this, whether cgi-type or not
std::string WebParser::extractRoot(size_t contextStart, size_t contextEnd) const
{
    std::string key = "root";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one 'root' directive per location context is allowed");
    if (directiveLocation == 0)
    {   
        if (locateDirective(contextStart, contextEnd, "alias") != 0 || locateDirective(contextStart, contextEnd, "proxy_pass") != 0
            || locateDirective(contextStart, contextEnd, "cgi_pass") != 0  || locateDirective(contextStart, contextEnd, "return") != 0)
            return ("");
        throw WebErrors::ConfigFormatException("Error: please add the 'root' directive to all location contexts that do not contain 'proxy_pass', 'cgi_pass', 'return' or 'alias' directives");
    }
    
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);

    //checking that the root starts with '/' (it should since we aren't using regular expressions?)
    if (line.length() == 0)
        throw WebErrors::ConfigFormatException("Error: root directive must have a value");
    if (line[0] != '/' && _servers.back().server_name.size() == 0)
        throw WebErrors::ConfigFormatException("Error: root directive's value must begin with '/'");

    //for now the only further error checking I'll do is regarding whitespaces
    size_t i;
    i = 0;
    while (line[i])
    {
        if (isspace(line[i]))
            throw WebErrors::ConfigFormatException("Error: root directive must only refer to one string value");
        i++;
    }
    return (line);
}

void    WebParser::extractAutoinex(size_t contextStart, size_t contextEnd)
{
    std::string key = "autoindex";
    ssize_t     directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one 'autoindex' directive per location context is allowed");
    if (directiveLocation == 0)
        return ;
    
    //An error should be thrown if the location context is cgi-type, but I don't know yet how we'll verify the type
    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    if (line.compare("on") == 0)
        _servers.back().locations.back().autoIndexOn = true;
    else if (line.compare("off") != 0)
        throw WebErrors::ConfigFormatException("Error: 'autoindex' may only have the value 'on' or 'off'");
}

void    WebParser::extractRedirectionAndTarget(size_t contextStart, size_t contextEnd)
{
    ssize_t     aliasLocation = locateDirective(contextStart, contextEnd, "alias");
    ssize_t     proxyLocation = locateDirective(contextStart, contextEnd, "proxy_pass");
    ssize_t     cgiLocation = locateDirective(contextStart, contextEnd, "cgi_pass");
    ssize_t     httpRedirLocation = locateDirective(contextStart, contextEnd, "return");

    if (aliasLocation == -1 || proxyLocation == -1 || cgiLocation == -1 || httpRedirLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one redirection type directive per location context is allowed");
    else if (aliasLocation != 0)
    {
        if (proxyLocation > 0 || cgiLocation  > 0 || httpRedirLocation > 0)
            throw WebErrors::ConfigFormatException("Error: only one type of redirection allowed per location context");
        //parse the alias, and store it in location.target
        _servers.back().locations.back().target = removeDirectiveKey(_configFile[aliasLocation], "alias");
        if (!verifyTarget(_servers.back().locations.back().target))
            throw WebErrors::ConfigFormatException("Error: " + _servers.back().locations.back().target + " does not exist");
        _servers.back().locations.back().type = ALIAS;
        return ;
    }
    else if (proxyLocation != 0)
    {
        if (cgiLocation  > 0 || httpRedirLocation > 0)
            throw WebErrors::ConfigFormatException("Error: only one type of redirection allowed per location context");
        //parse the proxy_pass, and store it in location.target
        //no error checking is done yet
        _servers.back().locations.back().target = removeDirectiveKey(_configFile[proxyLocation], "proxy_pass");
        if (_servers.back().locations.back().target.size() == 0)
            throw WebErrors::ConfigFormatException("Error: proxy_pass directive cannot be empty");
        _servers.back().locations.back().type = PROXY;
        return ;
    }
    else if (cgiLocation != 0)
    {
        if (httpRedirLocation > 0)
            throw WebErrors::ConfigFormatException("Error: only one type of redirection allowed per location context");
        //parse the cgi_pass, and create target from server_root + cgi_pass
        //no error checking is done yet
        _servers.back().locations.back().target = removeDirectiveKey(_configFile[cgiLocation], "cgi_pass");
        if (_servers.back().locations.back().target.size() == 0)
            throw WebErrors::ConfigFormatException("Error: cgi_pass directive cannot be empty");
        if (_servers.back().locations.back().target[0] != '/')
            throw WebErrors::ConfigFormatException("Error: cgi_pass's value string must start with /");
        if (_servers.back().server_root.size() != 0)
            _servers.back().locations.back().target = createStandardTarget(_servers.back().locations.back().target, _servers.back().server_root);
        _servers.back().locations.back().type = CGI;
        return ;
    }
    else if (httpRedirLocation != 0)
    {
        //parse the cgi_pass, and create target from server_root + cgi_pass
        //no error checking is done yet
        _servers.back().locations.back().target = removeDirectiveKey(_configFile[httpRedirLocation], "return");
        if (_servers.back().locations.back().target.size() == 0)
            throw WebErrors::ConfigFormatException("Error: 'return' directive cannot be empty");
        _servers.back().locations.back().type = HTTP_REDIR;
        if (_servers.back().locations.back().target.find("https://")!= 0 && _servers.back().locations.back().target.find("http://") != 0)
            throw WebErrors::ConfigFormatException( "Error: 'return' directive must start with 'http://' or 'https://'" );
        return ;
    }
    //since there is no redirection, create location.target from server_root + location.root + location.uri
    _servers.back().locations.back().target = createStandardTarget(_servers.back().locations.back().uri, _servers.back().locations.back().root);
    if (_servers.back().server_root.size() != 0)
        _servers.back().locations.back().target = createStandardTarget(_servers.back().locations.back().target, _servers.back().server_root);
    if (!verifyTarget(_servers.back().locations.back().target))
        throw WebErrors::ConfigFormatException("Error: " + _servers.back().locations.back().target + " does not exist");
    _servers.back().locations.back().type = STANDARD;
}

void    WebParser::extractIndex(size_t contextStart, size_t contextEnd)
{
    std::string key = "index";
    ssize_t directiveLocation = locateDirective(contextStart, contextEnd, key);

    if (directiveLocation == -1)
        throw WebErrors::ConfigFormatException("Error: only one 'index' directive allowed per location context");
    //if no index is specified what should it be set to? It's just left empty for now
    if (directiveLocation == 0)
        return ;

    std::string line = removeDirectiveKey(_configFile[directiveLocation], key);
    if (line.length() == 0)
        return ;
    std::string subLine;
    std::istringstream  stream(line);

    while (getline(stream, subLine, ' '))
    {
        try
        {
            if (_servers.back().server_root.size() != 0)
                subLine = createStandardTarget(subLine, _servers.back().server_root);
            _servers.back().locations.back().index.push_back(subLine);
        }
        catch(const std::exception& e)
        {
            WebErrors::printerror("WebParser::extractIndex", e.what());
            throw WebErrors::BaseException("Vector operation failed");
        }
    }
}
