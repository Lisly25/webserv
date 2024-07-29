#pragma once

#include <iostream>
#include <exception>
#include <string>

#define ARG_ERROR "\nInvalid amount of arguments! Run like this --> ./webserv [configuration file]\n"

namespace WebErrors
{
    class BaseException : public std::exception
    {
    public:
        explicit BaseException(const std::string &message) : _message(message) {}

        virtual const char* what() const noexcept override
        {
            return _message.c_str();
        }

    private:
        std::string _message;
    };

    class FileOpenException : public BaseException {
    public:
        explicit FileOpenException(const std::string &filename)
            : BaseException("Error opening config file: " + filename) {}
    };

    inline int printerror(const std::string &e)
    {
        std::cerr << e << std::endl;
        return -1;
    }
}
