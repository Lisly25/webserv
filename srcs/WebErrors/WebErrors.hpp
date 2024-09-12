#pragma once

#include <iostream>
#include <exception>
#include <netdb.h>
#include <string>
#include <errno.h>

#define ARG_ERROR "\nInvalid amount of arguments! Run like this --> ./webserv [configuration file]\n"

class WebServer;

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

    class FileOpenException : public BaseException
    {
    public:
        explicit FileOpenException(const std::string &filename);
    };


    class ConfigFormatException : public BaseException
    {
    public:
        explicit ConfigFormatException(std::string error_msg);
    };

     class ProxyException : public BaseException
    {
    public:
        explicit ProxyException(const std::string &message);
    };

    class ServerException : public BaseException
    {
    public:
        explicit ServerException(const std::string &message);

    };
    class SocketException : public BaseException
    {
    public:
        explicit SocketException(const std::string &message);
    };
    int printerror(const std::string &location, const std::string &e);
    void combineExceptions(const std::exception &original, const std::exception &inner);
}
