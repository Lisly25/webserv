#include "WebErrors.hpp"
#include <cstring>
#include "WebServer.hpp"

namespace WebErrors
{
    BaseException::BaseException(const std::string &message) : _message(message) {}

    const char* BaseException::what() const noexcept
    {
        return _message.c_str();
    }

    FileOpenException::FileOpenException(const std::string &filename)
        : BaseException("Error opening config file: " + filename) {}

    ConfigFormatException::ConfigFormatException(std::string error_msg)
        : BaseException(error_msg) {}

    /* Handles exceptions happening in the client handling in server */
    ProxyException::ProxyException(const std::string &message)
        : BaseException(message) { }

    int printerror(const std::string &e)
    {
        if (errno != 0)
            std::cerr << e << ": " << strerror(errno) << std::endl;
        else
            std::cerr << e << std::endl;
        return (EXIT_FAILURE);
    }

    void combineExceptions(const std::exception &original, const std::exception &inner)
    {
        std::string combined_error = "Recovery error: " + std::string(inner.what()) + 
                                    "; Initial error: " + std::string(original.what());
        throw ServerException(combined_error);
    }

    /* Server Exceptions in the server */
    ServerException::ServerException(const std::string &message)
        : BaseException(message) { }

    SocketException::SocketException(const std::string &message)
        : BaseException(message) { }
}
