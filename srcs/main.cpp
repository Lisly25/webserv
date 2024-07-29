#include "WebServer.hpp"
#include "WebErrors.hpp"
#include <fstream>
#include "WebParser/WebParser.hpp"

int main(int ac, char **av)
{
    if (ac == 2)
    {
        try
        {
            WebParser parser(av[1]);
            parser.parse();
            std::cout << "Passes Here --> " << parser.getCgiPass() << " " << parser.getProxyPass() << "\n\n";
        }
        catch (std::exception &e) {
            WebErrors::printerror(e.what());
        }
    }
    else
        return (WebErrors::printerror(ARG_ERROR));
    return (0);
}
