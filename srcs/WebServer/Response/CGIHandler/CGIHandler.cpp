#include "CGIHandler.hpp"
#include "WebParser.hpp"
#include <fcntl.h>
#include "WebServer.hpp"
#include "ErrorHandler.hpp"

CGIHandler::CGIHandler(const Request& request, WebServer &webServer) : _webServer(webServer), _request(request), _cgiInfo(), _path(_request.getRequestData().uri)
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
        // Close unused pipe ends
        close(_input_pipe[READEND]);
        close(_input_pipe[WRITEND]);
        
        // Add output pipe (READEND) to the main event loop (e.g., epoll)
        _cgiInfo.readEndFd = _output_pipe[READEND];
        _cgiInfo.pid = pid;
        _cgiInfo.startTime = std::time(nullptr);
        _cgiInfo.clientSocket = _webServer.getCurrentEventFd();

        // Add the read end of the pipe to the epoll loop (non-blocking)
        _webServer.epollController(_cgiInfo.readEndFd, EPOLL_CTL_ADD, EPOLLIN);

        // Store CGI process information in the server for future monitoring
        _webServer.getCgiInfos().push_back(_cgiInfo);
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI: Exception in parent process: " << e.what() << "\n";
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
