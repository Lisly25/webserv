#include "WebParser.hpp"

std::string WebParser::removeDirectiveKey(std::string line, std::string key)
{
    size_t valueIndex = line.find(key) + key.length();

    while (isspace(line[valueIndex]))
        valueIndex++;
    line = line.substr(valueIndex, line.length() - valueIndex - 1);
	return (line);
}