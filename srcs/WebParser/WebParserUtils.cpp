#include "WebParser.hpp"
#include "WebErrors.hpp"

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

bool    WebParser::locateServerContextStart(std::string line, std::string contextName)
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
    return (true);
}

bool    WebParser::locateLocationContextStart(std::string line, std::string contextName)
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
    if (line[i] != '/')
        return (false);
    size_t uriStart = i;

    while (i < line.length() && !(isspace(line[i])))
        i++;
    if (i == line.length() || i == uriStart)
        return (false);
    while (isspace(line[i]))
        i++;
    if (line[i] == '{')
        i++;
    if (i != line.length())
        return (false);
    return (true);
}

//also removes the semicolon from the end of the directive
std::string WebParser::removeDirectiveKey(std::string line, std::string key)
{
    size_t valueIndex = line.find(key) + key.length();

    while (isspace(line[valueIndex]))
        valueIndex++;
    line = line.substr(valueIndex, line.length() - valueIndex - 1);
	return (line);
}

std::string WebParser::createStandardTarget(std::string uri, std::string root)
{
    //could check here if the uri part ends with a '/' -> in nginx, it is important that it does, but
    //I'm not sure it should be for us?
    //Also should the root end with '/', too?

    //we already checked that the root starts with '/', and if a location URI does not start with '/', it is not even considered a location
    if (*(root.end()) == '/')
        root = root.substr(0, (root.length() - 1));
    return (root + uri);
}

bool WebParser::verifyTarget(std::string path)
{
    const char *ptr = path.c_str();

    if (access(ptr, F_OK) == -1)
        return (false);
    return (true);
}

std::vector<std::string>    WebParser::generateIndexPage(std::string path)
{
    std::vector<std::string>    IndexPageBody;

    IndexPageBody.push_back("<!doctype html>\n");
    IndexPageBody.push_back("<html lang=\"en-US\">\n");
    IndexPageBody.push_back("<head>\n");
    IndexPageBody.push_back("\t<meta charset=\"UTF-8\" />\n");
    IndexPageBody.push_back("\t<style>\n");
    IndexPageBody.push_back("\t\th1 {\n");
    IndexPageBody.push_back("\t\t\ttext-align: center;\n");
    IndexPageBody.push_back("\t\t\tfont-size: xxx-large;\n");
    IndexPageBody.push_back("\t\t}\n");
    IndexPageBody.push_back("\n");
    IndexPageBody.push_back("\t\t#link {\n");
    IndexPageBody.push_back("\t\t\tfont-size: xx-large;\n");
    IndexPageBody.push_back("\t\t}\n");
    IndexPageBody.push_back("\t</style>\n");
    IndexPageBody.push_back("</head>\n");
    IndexPageBody.push_back("<body>\n");
    IndexPageBody.push_back("\t<div>\n");
    IndexPageBody.push_back("\t\t<h1>AUTO-INDEXED LIST OF CONTENTS</h1>\n");
    IndexPageBody.push_back("\t</div>\n");
    IndexPageBody.push_back("\t<div id=\"link\">\n");

    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::filesystem::path   filepath = entry.path();
        std::string link = "\t\t<a href=\"" + std::string(filepath.filename()) + "\">" + std::string(filepath.filename()) + "</a><br>\n";
        IndexPageBody.push_back(link);
    }

    IndexPageBody.push_back("\t</div>\n");
    IndexPageBody.push_back("</body>\n");
    IndexPageBody.push_back("</html>");
    return (IndexPageBody);
}

//For testing!

void    WebParser::printAutoIndexToFile(void)
{
    std::vector<std::string>    AutoIndexBody = generateIndexPage("/mnt/c/Hive/webserv/");
    std::ofstream   outfile;
    outfile.open("autoindex.html", std::ios::trunc);
    for (size_t i = 0; i < AutoIndexBody.size(); i++)
    {
        outfile << AutoIndexBody[i];
    }
    outfile.close();
}

int     WebParser::getErrorCode(std::string line)
{
    std::stringstream stream(line);
    int         errorCode;

    stream >> errorCode;
    if (stream.fail())
        throw WebErrors::ConfigFormatException("Error: missing error code in error_page directive");
    if (errorCode < 400 || errorCode > 599)
        throw WebErrors::ConfigFormatException("Error: default error page was specified for invalid error code. Valid range is 400 - 599");
    
    std::string leftover;
    stream >> leftover;
    if (leftover.size() != 0)
        throw WebErrors::ConfigFormatException("Error: error_page directive must be a single pair of error code and error page URI");
    return (errorCode);
}

//Returns an empty string if no page is found / can't access() the page
std::string   WebParser::getErrorPage(int errorCode, Server server)
{
    std::string errorPage;
    try
    {
        errorPage = server.error_page.at(errorCode);
    }
    catch(const std::out_of_range& e)
    {
        std::cerr << "No error page for code " << errorCode << '\n';
        return ("");
    }
    if (access(errorPage.c_str(), R_OK) == -1)
    {
        std::cerr << "Error: error_page has no write permission" << std::endl;
        return ("");
    }
    return (errorPage);
}
