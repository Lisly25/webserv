
#include "CGIHandler.hpp"

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
    pid_t pid;

    if (pipe(_output_pipe) == -1 || pipe(_input_pipe) == -1)
    {
        _response = CODE500;
        return;
    }
    pid = fork();
    if (pid < 0)
    {
        _response = CODE500;
        return;
    }
    else if (pid == 0)
    {
        child();
    }
    else
    {
        parent(pid);
    }
}

void CGIHandler::child(void)
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

    setEnvp(envp);

    execve(PYTHON3, (char *const *)argv, (char *const *)envp);

    std::cerr << "execve failed.\n";
    exit(1);
}

void CGIHandler::parent(pid_t pid)
{
    char    buffer[4096];
    int     bytes;

    close(_input_pipe[READEND]);
    write(_input_pipe[WRITEND], _request.getRequestData().body.c_str(), _request.getRequestData().body.size());
    close(_input_pipe[WRITEND]);
    close(_output_pipe[WRITEND]);

    if (!waitForChild(pid))
    {
        _response = CODE500;
        close(_output_pipe[READEND]);
        return;
    }
    while ((bytes = read(_output_pipe[READEND], buffer, sizeof(buffer))) > 0)
    {
        _response.append(buffer, bytes);
    }
    if (bytes == -1)
    {
        _response = CODE500;
    }
    close(_output_pipe[READEND]);
}

void CGIHandler::setEnvp(char const *envp[])
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

bool CGIHandler::waitForChild(pid_t pid)
{
    int status;
    const int timeout = 30;
    double elapsed;
    pid_t retPid;
    clock_t start = std::clock();
    clock_t current;

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
