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
        explicit BaseException(const std::string &message);

        virtual const char* what() const noexcept override;

    private:
        std::string _message;
    };

    class FileOpenException : public BaseException {
    public:
        explicit FileOpenException(const std::string &filename);
    };

    class ConfigFormatException : public BaseException {
    public:
        explicit ConfigFormatException(std::string error_msg);
    };

    int printerror(const std::string &e);
}
