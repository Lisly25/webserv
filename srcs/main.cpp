#include "WebServer.hpp"
#include "WebErrors.hpp"
#include <fstream>
#include "ConfigParser/ConfigParser.hpp"

int main(int ac, char **av)
{
    if (ac == 2)
    {
        try
        {
            ConfigParser parser(av[1]);
            parser.parse();
            std::cout << "Passes Here --> " << parser.getCgiPass() << " " << parser.getProxyPass() << "\n\n";
        }
        catch (std::exception &e) {
            WebErrors::printerror(e.what());
        }
    }
    else
        return (WebErrors::printerror(ARG_ERROR));
    std::cout << "helloworld\n";
    return (0);
}
