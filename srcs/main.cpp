#include "WebServer.hpp"
#include "WebErrors.hpp"
#include <fstream>
#include "ConfigParser/ConfigParser.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
        return (WebErrors::printerror(ARG_ERROR));
    else
    {
        std::ifstream file(av[1]);
        if (!file.is_open())
            return (WebErrors::printerror(ARG_ERROR));
    }
    std::cout << "helloworld\n";
    return (0);
}
