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
        
            // Ports below 1024 require root access so use above 
            // The occupied ports can be checked from cat /etc/services
            // Modify the listen port from the conf and the server_name
            WebServer server(parser);
            server.start();
        }
        catch (std::exception &e) {
            WebErrors::printerror(e.what());
        }
    }
    else
        return (WebErrors::printerror(ARG_ERROR));
    return (0);
}
