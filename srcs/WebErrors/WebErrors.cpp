#include "WebErrors.hpp"

namespace WebErrors
{
    BaseException::BaseException(const std::string &message) : _message(message) {}

    const char* BaseException::what() const noexcept
    {
        return _message.c_str();
    }

    FileOpenException::FileOpenException(const std::string &filename)
        : BaseException("Error opening config file: " + filename) {}

    int printerror(const std::string &e)
    {
        std::cerr << e << std::endl;
        return -1;
    }
}
