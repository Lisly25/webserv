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

     class ClientException : public BaseException
    {
    public:
        ClientException(const std::string &message, addrinfo* res = nullptr,\
            WebServer* server = nullptr, int clientSocket = -1);
        ~ClientException();

    private:
        addrinfo*   _res;
        WebServer*  _server;
        int         _clientSocket;
    };

    class ServerException : public BaseException
    {
    public:
        explicit ServerException(const std::string &message);
        ~ServerException();

    };

    int printerror(const std::string &e);
}
