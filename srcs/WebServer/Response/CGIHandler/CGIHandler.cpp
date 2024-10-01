
#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
#include "WebErrors.hpp"
#include "WebParser.hpp"
#include "WebServer.hpp"
#include <fcntl.h>

CGIHandler::CGIHandler(const Request& request, WebServer &webServer) : _webServer(webServer), _request(request), _response(""), _scriptPath(_request.getRequestData().uri)
{
    std::cout << COLOR_YELLOW_CGI << "  CGIHandler: " << _request.getRequestData().method << " " <<  " 🐍\n\n" << COLOR_RESET;
    executeScript();
};

void CGIHandler::executeScript(void)
{
    try
    {
        pid_t   pid;

        if (access(_scriptPath.c_str(), R_OK) != 0)
        {
            ErrorHandler(_request).handleError(_response, 500);
            return WebErrors::printerror("CGIHandler::executeScript", "Error accessing script file") , void();
        }
        if (pipe(_fromCgi_pipe) == -1 || pipe(_toCgi_pipe) == -1)
        {
            ErrorHandler(_request).handleError(_response, 500);
            return WebErrors::printerror("CGIHandler::executeScript", "Error creating pipes") , void();
        }
        WebServer::setFdNonBlocking(_fromCgi_pipe[READEND]);
        WebServer::setFdNonBlocking(_fromCgi_pipe[WRITEND]);
        pid = fork();
        if (pid < 0)
        {
            ErrorHandler(_request).handleError(_response, 500);
            return WebErrors::printerror("CGIHandler::executeScript", "Error forking process") , void();
        }
        else if (pid == 0)
            child();
        else
            parent(pid);
        std::cout << COLOR_YELLOW_CGI << "  CGI Script Started 🐍\n\n" << COLOR_RESET;
    }
    catch (const std::exception &e)
    {
        ErrorHandler(_request).handleError(_response, 500);
        WebErrors::printerror("CGIHandler::executeScript", e.what());
    }
}

void CGIHandler::child(void)
{
    try
    {
        size_t      lastSlashPos = _scriptPath.find_last_of('/');
        std::string scriptDir;

        if (lastSlashPos != std::string::npos)
            scriptDir = _scriptPath.substr(0, lastSlashPos);
        else
            scriptDir = ".";
        if (chdir(scriptDir.c_str()) != 0)
        {
            std::cout << WebParser::getErrorPage(500, _request.getServer());
            exit(EXIT_FAILURE);
        }
        char const *argv[] = {PYTHON3, _scriptPath.c_str(), NULL};
        char const *envp[9];

        close(_toCgi_pipe[WRITEND]);
        dup2(_toCgi_pipe[READEND], STDIN_FILENO);
        close(_toCgi_pipe[READEND]);

        close(_fromCgi_pipe[READEND]);
        dup2(_fromCgi_pipe[WRITEND], STDOUT_FILENO);
        dup2(_fromCgi_pipe[WRITEND], STDERR_FILENO);
        close(_fromCgi_pipe[WRITEND]);

        childSetEnvp(envp);
        execve(PYTHON3, (char *const *)argv, (char *const *)envp);

        WebErrors::printerror("CGIHandler::child", "Error executing script");
        std::cout << WebParser::getErrorPage(500, _request.getServer());
        std::terminate();
    }
    catch (const std::exception &e)
    {
        ErrorHandler(_request).handleError(_response, 500);
        std::cout << _response << std::endl;
        exit(EXIT_FAILURE);
    }
}


void CGIHandler::parent(pid_t pid)
{
    try
    {
        CGIProcessInfo cgiInfo;

        cgiInfo.pid = pid;
        cgiInfo.clientSocket = _webServer.getCurrentEventFd();
        cgiInfo.response = "";
        cgiInfo.startTime = std::chrono::steady_clock::now();
        cgiInfo.readFromCgiFd = _fromCgi_pipe[READEND];
        cgiInfo.writeToCgiFd = _toCgi_pipe[WRITEND];
        if (_request.getRequestData().method == "POST" && !_request.getRequestData().body.empty())
        {
            const size_t bodySize = _request.getRequestData().body.size();
            const ssize_t written = write(_toCgi_pipe[WRITEND], _request.getRequestData().body.c_str(), bodySize);
            if (written == -1)
                throw std::runtime_error("Failed to write to CGI script");
            else if (written == 0)
                throw std::runtime_error("Zero bytes written to CGI");
        }
        else
            close(_toCgi_pipe[WRITEND]);
        _webServer.getCgiInfoList().push_back(cgiInfo);
        _webServer.epollController(_fromCgi_pipe[READEND], EPOLL_CTL_ADD, EPOLLIN, FdType::CGI_PIPE);
        close(_toCgi_pipe[READEND]);
        close(_fromCgi_pipe[WRITEND]);
    }
    catch (const std::exception &e)
    {
        ErrorHandler(_request).handleError(_response, 500);
    }
}


void CGIHandler::childSetEnvp(char const *envp[])
{
    try
    {
        static std::vector<std::string> env(9);
        const RequestData *reqData =    &_request.getRequestData();

        env[0] = "REQUEST_METHOD=" + reqData->method;
        env[1] = "QUERY_STRING=" + reqData->query_string;
        env[2] = "CONTENT_TYPE=" + reqData->content_type;
        env[3] = "CONTENT_LENGTH=" + reqData->content_length;
        env[4] = "DOCUMENT_ROOT=" + reqData->absoluteRootPath;
        env[5] = "SCRIPT_FILENAME=" + _scriptPath;
        env[6] = "SCRIPT_NAME=" + _scriptPath;
        env[7] = "REDIRECT_STATUS=200";
        env[8] = "UPLOAD_FOLDER="  + _request.getLocation()->upload_folder;

        envp[0] = env[0].c_str();
        envp[1] = env[1].c_str();
        envp[2] = env[2].c_str();
        envp[3] = env[3].c_str();
        envp[4] = env[4].c_str();
        envp[5] = env[5].c_str();
        envp[6] = env[6].c_str();
        envp[7] = env[7].c_str();
        envp[8] = env[8].c_str();
        envp[9] = NULL;
    }
    catch (const std::exception &e)
    {
        ErrorHandler(_request).handleError(_response, 500);
        std::cout <<  _response << std::endl;
        exit(EXIT_FAILURE);
    }
}
