
#include "CGIHandler.hpp"

CGI::CGI(Request& request, std::string& response) : _request(request), _response(response), _path(_request.getLocation()->target)
{
    if (validateExecutable() == false)
        return;
    executeScript();
};

bool    CGI::validateExecutable( void )
{
    if (access(_path.c_str(), F_OK) < 0)
    {
        _response = CODE404;
        return false;
    }
    if (access(_path.c_str(), R_OK | X_OK) < 0)
    {
        _response = CODE500;
        return false;
    }
    return true;
};

void    CGI::executeScript( void )
{
    pid_t   pid;

    if (pipe(_pipe) == -1)
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
        parent( pid );
    }
};

void    CGI::setEnvp( char const *envp[] )
{
    std::vector<std::string>    env;
    std::string 	        query;

    env.resize(9);
    env[0] = "REQUEST_METHOD=";
    env[1] = "QUERY_STRING=";
    env[2] = "CONTENT_TYPE=text/html";
    env[3] = "CONTENT_LENGTH=";
    env[4] = "DOCUMENT_ROOT=";
    env[5] = "SCRIPT_FILENAME=" + _path;
    env[6] = "SCRIPT_NAME=" + _path;
    env[7] = "REDIRECT_STATUS=200";
    env[8] = "REQUEST_BODY=";

    envp[0] = env[0].c_str();
    envp[1] = env[1].c_str();
    envp[2] = env[2].c_str();
    envp[3] = env[3].c_str();
    envp[4] = env[4].c_str();
    envp[5] = env[5].c_str();
    envp[6] = env[6].c_str();
    envp[7] = env[7].c_str();
    envp[8] = env[8].c_str();
};

void    CGI::child( void )
{
    char    const   *argv[] = {PYTHON3, _path.c_str(), NULL};
    char    const   *envp[9];

    setEnvp( envp );
    if (dup2(_pipe[WRITEND], WRITEND) == -1)
    {
        std::cout << ERROR << "\033[32dup2 failed.\033[0m\n";
        _response = CODE500;
        return;
    }
    if (dup2(_pipe[READEND], READEND) == -1)
    {
        std::cout << ERROR << "\033[32dup2 failed.\033[0m\n";
        _response = CODE500;
        return;
    }
    if (execve(PYTHON3, (char *const *)argv, (char *const *)envp) == -1)
    {
        std::cout << ERROR << "\033[32execve failed.\033[0m\n";
        _response = CODE500;
        return;
    }
};

bool    waitForChild( pid_t pid )
{
    int status;
    const   int timeout = 7;
    double  elapsed;
    pid_t   retPid;
    clock_t start;
    clock_t current;
    start = std::clock();
    while (true)
    {
        current = std::clock();
        elapsed = static_cast<double>(current - start) / CLOCKS_PER_SEC;
        retPid = waitpid(pid, &status, WNOHANG);
        if (retPid == -1)
        {
            std::cout << ERROR << "\033[32execve failed.\033[0m\n";
            return false;
        }
        if (retPid > 0)
            break;
        if (elapsed > timeout)
        {
            std::cout << ERROR << "\033[32CGI timeout.\033[0m\n";
            return false;
        }

    }
    return true;
};

void    CGI::parent( pid_t pid )
{
    close(_pipe[WRITEND]);
    if (waitForChild( pid ) == false)
    {
        _response = CODE500;
        close(_pipe[READEND]);
        return;
    }
    char    buffer[4096];
    memset(buffer, 0, 4096);
    int bytes = 1;
    while (bytes)
    {
        bytes = read(_pipe[READEND], buffer, 4096);
        if (bytes == -1)
        {
            std::cout << ERROR << "\033[32read failed.\033[0m\n";
            _response = CODE500;
            return;
        }
        _response += buffer;
        memset(buffer, 0, 4096);
    }
    close(_pipe[READEND]);
};

std::string&    CGI::getCGIResponse( void ) const
{
    return _response;
}
