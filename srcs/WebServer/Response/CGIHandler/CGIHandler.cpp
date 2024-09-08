
#include "CGIHandler.hpp"
#include "WebParser.hpp"
#include <fcntl.h>

CGIHandler::CGIHandler(const Request& request) : _request(request), _response(""), _path(_request.getRequestData().uri)
{
    std::cout << "\033[31mIN CGI: going to RUN IT\033[0m\n";
    std::cout << "\033[37mWITH: " << _path << "\033[0m\n";
    std::cout << "\033[37mWITH METHOD: " << _request.getRequestData().method << "\033[0m\n";
    std::cout << "\033[37mWITH CONTENT_LENGTH: " << _request.getRequestData().content_length << "\033[0m\n";
};

static void nonBlockFd(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        throw std::system_error();
    flags |= O_NONBLOCK;
    int res = fcntl(fd, F_SETFL, flags);
    if (res == -1)
        throw std::system_error();
}

std::pair<pid_t, int> CGIHandler::executeScript(void)
{
    try
    {
        pid_t pid;

        if (pipe(_output_pipe) == -1 || pipe(_input_pipe) == -1)
        {
            _response = WebParser::getErrorPage(500, _request.getServer());
            std::cerr << "CGI: Failed to create pipes.\n";
            return std::make_pair(-1, -1);
        }
        nonBlockFd(_output_pipe[READEND]);
        nonBlockFd(_output_pipe[WRITEND]);
        nonBlockFd(_input_pipe[READEND]);
        nonBlockFd(_input_pipe[WRITEND]);
        pid = fork();
        if (pid < 0)
        {
            _response = WebParser::getErrorPage(500, _request.getServer());
            std::cerr << "CGI: Fork failed.\n";
            return std::make_pair(-1, -1);
        }
        else if (pid == 0)
        {
            child();
            std::terminate();
        }
        else
        {
            return parent(pid);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI: Exception caught during script execution: " << e.what() << std::endl;
        _response = WebParser::getErrorPage(500, _request.getServer());
        return std::make_pair(-1, -1);  // Return invalid pair on failure
    }
}



void CGIHandler::child(void)
{
    try
    {
        char const *argv[] = {PYTHON3, _path.c_str(), NULL};
        char const *envp[9];

        close(_input_pipe[1]);
        dup2(_input_pipe[0], STDIN_FILENO);
        close(_input_pipe[0]);

        close(_output_pipe[0]);
        dup2(_output_pipe[1], STDOUT_FILENO);
        dup2(_output_pipe[1], STDERR_FILENO);
        close(_output_pipe[1]);

        childSetEnvp(envp);
        execve(PYTHON3, (char *const *)argv, (char *const *)envp);

        std::cout <<  WebParser::getErrorPage(500, _request.getServer());
        std::terminate();
    }
    catch (const std::exception &e)
    {
        std::cout <<  WebParser::getErrorPage(500, _request.getServer());
        exit(EXIT_FAILURE);
    }
}

std::pair<pid_t, int> CGIHandler::parent(pid_t pid)
{
    try
    {
        close(_input_pipe[READEND]);
        size_t bodySize = _request.getRequestData().body.size();
        ssize_t written = write(_input_pipe[WRITEND], _request.getRequestData().body.c_str(), bodySize);
        if (written == -1 || static_cast<size_t>(written) != bodySize)
        {
            throw std::runtime_error("Failed to write complete CGI input pipe data.");
        }
        close(_input_pipe[WRITEND]);
        close(_output_pipe[WRITEND]);
        return std::make_pair(pid, _output_pipe[READEND]);
    }
    catch (const std::exception &e)
    {
        _response = WebParser::getErrorPage(500, _request.getServer());
        throw ;
    }
}

void CGIHandler::childSetEnvp(char const *envp[])
{
    try
    {
        static std::vector<std::string> env(8);
        const RequestData *reqData =    &_request.getRequestData();

        env[0] = "REQUEST_METHOD=" + reqData->method;
        env[1] = "QUERY_STRING=" + reqData->query_string;
        env[2] = "CONTENT_TYPE=" + reqData->content_type;
        env[3] = "CONTENT_LENGTH=" + reqData->content_length;
        env[4] = "DOCUMENT_ROOT=" + reqData->absoluteRootPath;
        env[5] = "SCRIPT_FILENAME=" + _path;
        env[6] = "SCRIPT_NAME=" + _path;
        env[7] = "REDIRECT_STATUS=200";

        envp[0] = env[0].c_str();
        envp[1] = env[1].c_str();
        envp[2] = env[2].c_str();
        envp[3] = env[3].c_str();
        envp[4] = env[4].c_str();
        envp[5] = env[5].c_str();
        envp[6] = env[6].c_str();
        envp[7] = env[7].c_str();
        envp[8] = NULL;
    }
    catch (const std::exception &e)
    {
        std::cout <<  WebParser::getErrorPage(500, _request.getServer());
        exit(EXIT_FAILURE);
    }
}



std::string CGIHandler::getCGIResponse(void) const
{
    return _response;
}
