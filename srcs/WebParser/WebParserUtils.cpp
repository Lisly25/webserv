#include "WebParser.hpp"

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