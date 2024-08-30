
#include "CGIHandler.hpp"
#include "WebParser.hpp"

CGIHandler::CGIHandler(const Request& request) : _request(request), _response(""), _path(_request.getRequestData().uri)
{
    std::cout << "\033[31mIN CGI: going to RUN IT\033[0m\n";
    std::cout << "\033[37mWITH: " << _path << "\033[0m\n";
    std::cout << "\033[37mWITH METHOD: " << _request.getRequestData().method << "\033[0m\n";
    std::cout << "\033[37mWITH CONTENT_LENGTH: " << _request.getRequestData().content_length << "\033[0m\n";
    executeScript();
};

void CGIHandler::executeScript(void)
{
    try
    {
        pid_t pid;

        if (pipe(_output_pipe) == -1 || pipe(_input_pipe) == -1)
            return (std::cerr << "CGI: Failed to create pipes.\n", void());
        pid = fork();
        if (pid < 0)
            return (std::cerr << "CGI: Fork failed.\n", void());
        else if (pid == 0)
            child();
        else
            parent(pid);
    }
    catch (const std::exception &e)
    {
        std::cerr << "CGI: Error in CGIHandler: " << e.what() << std::endl;
        _response = WebParser::getErrorPage(500, _request.getServer());
    }
}

void CGIHandler::child(void)
{
    std::ofstream logFile("cgi_error.log", std::ios::app);
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

        logFile << "CGI: child execve failed.\n";
        exit(EXIT_FAILURE);
    }
    catch (const std::exception &e)
    {
        logFile << "CGI: Error in child process: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void CGIHandler::parent(pid_t pid)
{
    std::ofstream logFile("cgi_error.log", std::ios::app);
    try
    {
        char buffer[4096];
        int bytes;
        size_t totalBytesWritten = 0;
        size_t totalBytesRead = 0;

        close(_input_pipe[READEND]);
        size_t bodySize = _request.getRequestData().body.size();
        logFile << "CGI: Body size to be written: " << bodySize << " bytes." << std::endl;
        ssize_t written = write(_input_pipe[WRITEND], _request.getRequestData().body.c_str(), bodySize);
        if (written == -1 || static_cast<size_t>(written) != bodySize) {
            throw std::runtime_error("Failed to write complete CGI input pipe data.");
        }
        totalBytesWritten += bodySize;
        logFile << "CGI: Total bytes written: " << totalBytesWritten << " bytes." << std::endl;
        close(_input_pipe[WRITEND]);
        close(_output_pipe[WRITEND]);

        if (!parentWaitForChild(pid))
        {
            throw std::runtime_error("CGI process timed out.");
        }

        while ((bytes = read(_output_pipe[READEND], buffer, sizeof(buffer))) > 0)
        {
            totalBytesRead += bytes;
            _response.append(buffer, bytes);
        }
        logFile << "CGI: Total bytes read: " << totalBytesRead << " bytes." << std::endl;

        if (bytes == -1)
        {
            throw std::runtime_error("Failed to read CGI output.");
        }

        close(_output_pipe[READEND]);
    }
    catch (const std::exception &e)
    {
        logFile << "CGI: Error in parent process: " << e.what() << std::endl;
        _response = WebParser::getErrorPage(500, _request.getServer());
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
        std::cerr << "CGI: Error setting environment variables: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

bool CGIHandler::parentWaitForChild(pid_t pid)
{
    int         status;
    const int   timeout = 10;
    double      elapsed;
    pid_t       retPid;
    clock_t     start = std::clock();
    clock_t     current;

    while (true)
    {
        current = std::clock();
        elapsed = static_cast<double>(current - start) / CLOCKS_PER_SEC;
        retPid = waitpid(pid, &status, WNOHANG);
        if (retPid == -1)
        {
            std::cerr << "execve failed.\n";
            return false;
        }
        if (retPid > 0)
            break;
        if (elapsed > timeout)
        {
            std::cerr << "CGI timeout.\n";
            return false;
        }
    }
    return true;
}

std::string CGIHandler::getCGIResponse(void) const
{
    return _response;
}
