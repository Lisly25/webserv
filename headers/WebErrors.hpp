#pragma once 

#include <iostream>

#define ARG_ERROR "Invalid amount of arguments! Run like this --> \
./webserv [configuration file]\n"

class WebErrors
{
    public:
        static int printerror(const std::string &e)
        {
            std::cerr << e << std::endl;
            return (-1);
        }
};
