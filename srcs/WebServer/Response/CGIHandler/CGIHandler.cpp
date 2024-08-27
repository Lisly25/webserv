
#include "CGIHandler.hpp"

CGIHandler::CGIHandler(const Request& request) : _request(request), _response(""), _path(_request.getRequestData().uri)
{
    /*if (validateExecutable() == false)*/
    /*    return;*/
    RequestData reqData = _request.getRequestData();
    std::cout << "\033[31mIN CGI: going to RUN IT\033[0m\n";
    std::cout << "\033[37mWITH: " << _path << "\033[0m\n";
    std::cout << "\033[37mWITH METHOD: " << reqData.method << "\033[0m\n";
    std::cout << "\033[37mWITH QUERY_STRING: " << reqData.query_string << "\033[0m\n";
    executeScript();
};

bool    CGIHandler::validateExecutable( void )
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

void    CGIHandler::executeScript( void )
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

void    CGIHandler::setEnvp( char const *envp[] )
{
    static  std::vector<std::string>    env(10);
    std::string 	        query;
    RequestData reqData = _request.getRequestData();

    env[0] = "REQUEST_METHOD=DELETE";
    env[1] = "QUERY_STRING=" + reqData.query_string;
    env[2] = "CONTENT_TYPE=" + reqData.content_type;
    env[3] = "CONTENT_LENGTH=" + reqData.content_length;
    env[4] = "DOCUMENT_ROOT=" + reqData.absoluteRootPath;
    env[5] = "SCRIPT_FILENAME=" + _path;
    env[6] = "SCRIPT_NAME=" + _path;
    env[7] = "REDIRECT_STATUS=200";
    env[8] = "REQUEST_BODY=" + reqData.body;

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
};

void    CGIHandler::child( void )
{
    char    const   *argv[] = {PYTHON3, _path.c_str(), NULL};
    char    const   *envp[10];

    setEnvp( envp );
    if (dup2(_pipe[WRITEND], WRITEND) == -1)
    {
        std::cout << ERROR << "\033[32mdup2 failed.\033[0m\n";
        _response = CODE500;
        return;
    }
    if (dup2(_pipe[READEND], READEND) == -1)
    {
        std::cout << ERROR << "\033[32mdup2 failed.\033[0m\n";
        _response = CODE500;
        return;
    }
    if (execve(PYTHON3, (char *const *)argv, (char *const *)envp) == -1)
    {
        std::cout << ERROR << "\033[32mexecve failed.\033[0m\n";
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
            std::cout << ERROR << "\033[32mexecve failed.\033[0m\n";
            return false;
        }
        if (retPid > 0)
            break;
        if (elapsed > timeout)
        {
            std::cout << ERROR << "\033[32mCGI timeout.\033[0m\n";
            return false;
        }

    }
    return true;
};

void    CGIHandler::parent( pid_t pid )
{
    close(_pipe[WRITEND]);
    if (waitForChild( pid ) == false)
    {
        _response = CODE500;
        close(_pipe[READEND]);
        return;
    }
    std::cout << "here4\n";
    char    buffer[4096];
    memset(buffer, 0, 4096);
    int bytes = 1;
    while (bytes)
    {
        bytes = read(_pipe[READEND], buffer, 4096);
        if (bytes == -1)
        {
            std::cout << ERROR << "\033[32mread failed.\033[0m\n";
            _response = CODE500;
            return;
        }
        _response += buffer;
        memset(buffer, 0, 4096);
    }
    close(_pipe[READEND]);
};

std::string    CGIHandler::getCGIResponse( void ) const
{
    return _response;
}
