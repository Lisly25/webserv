#include "WebServer.hpp"
#include "Errors.hpp"
#include <fstream>

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << ARG_ERROR;
        return (-1);
    }
    else
    {
        std::ifstream file(av[1]);
        if (!file.is_open())
            return (Errorer::printerror(ARG_ERROR));
    }
    std::cout << "helloworld\n";
    return (0);
}
