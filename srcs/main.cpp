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

            WebServer server(parser);
            server.start();
        }
        catch (std::exception &e)
        {
            return (WebErrors::printerror("main()", ""));
        }
    }
    else
        return (WebErrors::printerror("main()", ARG_ERROR));
    return (EXIT_SUCCESS);
}
