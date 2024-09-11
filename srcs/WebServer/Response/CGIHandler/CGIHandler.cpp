#include "CGIHandler.hpp"
#include "WebParser.hpp"
#include <fcntl.h>

CGIHandler::CGIHandler(const Request& request) : _request(request), _cgiInfo(), _path(_request.getRequestData().uri)
{
    std::cout << "\033[31mIN CGI: going to RUN IT\033[0m\n";
    std::cout << "\033[37mWITH: " << _path << "\033[0m\n";
    std::cout << "\033[37mWITH METHOD: " << _request.getRequestData().method << "\033[0m\n";
    std::cout << "\033[37mWITH CONTENT_LENGTH: " << _request.getRequestData().content_length << "\033[0m\n";
}

CgiInfo CGIHandler::executeScript(void)
{
    _cgiInfo = CgiInfo();

    try
    {
        pid_t pid;

        if (access(_path.c_str(), R_OK) != 0)
        {
            std::fstream logfile("log.txt", std::ios::out | std::ios::app);
            logfile << "CGI: Script not found or not readable: " << strerror(errno) << "\n";
            logfile.close();
            _cgiInfo.error = true;
            return _cgiInfo;
        }

        if (pipe(_output_pipe) == -1 || pipe(_input_pipe) == -1)
        {
            std::fstream logfile("log.txt", std::ios::out | std::ios::app);
            logfile << "CGI: Failed to create pipes: " << strerror(errno) << "\n";
            logfile.close();
            _cgiInfo.error = true;
            return _cgiInfo;
        }

        nonBlockFd(_output_pipe[READEND]);
        nonBlockFd(_output_pipe[WRITEND]);
        nonBlockFd(_input_pipe[READEND]);
        nonBlockFd(_input_pipe[WRITEND]);

        pid = fork();
        if (pid < 0)
        {
            std::fstream logfile("log.txt", std::ios::out | std::ios::app);
            logfile << "CGI: Fork failed: " << strerror(errno) << "\n";
            logfile.close();
            _cgiInfo.error = true;
            return _cgiInfo;
        }
        else if (pid == 0)
        {
            child();
            std::terminate();
        }
        else
        {
            parent(pid);
            std::cout << "SUCCESS CGI \n";
             std::cout << "SUCCESS CGI \n";
              std::cout << "SUCCESS CGI \n";
               std::cout << "SUCCESS CGI \n";
                std::cout << "SUCCESS CGI \n";
            return _cgiInfo;
        }
    }
    catch (const std::exception &e)
    {
        std::fstream logfile("log.txt", std::ios::out | std::ios::app);
        logfile << "CGI: Exception during script execution: " << e.what() << "\n";
        logfile.close();
        _cgiInfo.error = true;
        return _cgiInfo;
    }
}

void CGIHandler::child(void)
{
    try
    {
        std::fstream logfile("log.txt", std::ios::out | std::ios::app);
        char const *argv[] = {PYTHON3, _path.c_str(), NULL};
        char const *envp[9];

        close(_input_pipe[WRITEND]);
        dup2(_input_pipe[READEND], STDIN_FILENO);
        close(_input_pipe[READEND]);

        close(_output_pipe[READEND]);
        dup2(_output_pipe[WRITEND], STDOUT_FILENO);
        dup2(_output_pipe[WRITEND], STDERR_FILENO);  // Redirect STDERR to log
        close(_output_pipe[WRITEND]);

        logfile << "CGI: Child process running script " << _path << "\n";
        logfile.close();

        childSetEnvp(envp);
        execve(PYTHON3, (char *const *)argv, (char *const *)envp);

        logfile.open("log.txt", std::ios::out | std::ios::app);
        logfile << "CGI: Failed to exec script: " << strerror(errno) << "\n";
        logfile.close();

        std::cout << WebParser::getErrorPage(500, _request.getServer());
        std::terminate();
    }
    catch (const std::exception &e)
    {
        std::fstream logfile("log.txt", std::ios::out | std::ios::app);
        logfile << "CGI: Exception in child process: " << e.what() << "\n";
        logfile.close();

        std::cout << WebParser::getErrorPage(500, _request.getServer());
        exit(EXIT_FAILURE);
    }
}

void CGIHandler::parent(pid_t pid)
{
    try
    {
        std::fstream logfile("log.txt", std::ios::out | std::ios::app);
        logfile << "CGI: Parent process handling PID: " << pid << "\n";

        close(_input_pipe[READEND]);
        size_t bodySize = _request.getRequestData().body.size();
        ssize_t written = write(_input_pipe[WRITEND], _request.getRequestData().body.c_str(), bodySize);
        if (written == -1 || static_cast<size_t>(written) != bodySize)
        {
            logfile << "CGI: Failed to write complete CGI input pipe data. Error: " << strerror(errno) << "\n";
            logfile.close();
            throw std::runtime_error("Failed to write complete CGI input pipe data.");
        }

        close(_input_pipe[WRITEND]);
        close(_output_pipe[WRITEND]);

        _cgiInfo.startTime = std::time(nullptr);
        _cgiInfo.pid = pid;
        _cgiInfo.readEndFd = _output_pipe[READEND];
        int new_fd = dup(_cgiInfo.readEndFd);
if (new_fd == -1) {
    std::cerr << "Failed to duplicate file descriptor: " << strerror(errno) << std::endl;
} else {
    _cgiInfo.readEndFd = new_fd;
}

        logfile << "READ END FD IN PARENT: " << _cgiInfo.readEndFd << "\n";
        logfile << "READ END FD IN PARENT: " << _cgiInfo.readEndFd << "\n";
        logfile << "READ END FD IN PARENT: " << _cgiInfo.readEndFd << "\n";
        logfile << "READ END FD IN PARENT: " << _cgiInfo.readEndFd << "\n";
        logfile << "READ END FD IN PARENT: " << _cgiInfo.readEndFd << "\n";
        logfile.close();
    }
    catch (const std::exception &e)
    {
        std::fstream logfile("log.txt", std::ios::out | std::ios::app);
        logfile << "Error in parent: " << e.what() << std::endl;
        logfile.close();

        std::cout << WebParser::getErrorPage(500, _request.getServer());
        throw;
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

void CGIHandler::nonBlockFd(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        throw std::system_error();
    flags |= O_NONBLOCK | O_CLOEXEC;
    int res = fcntl(fd, F_SETFL, flags);
    if (res == -1)
        throw std::system_error();
}
