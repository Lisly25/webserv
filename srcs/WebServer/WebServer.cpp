#include "WebServer.hpp"
#include "ScopedSocket.hpp"
#include "ScopedSocket/ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

bool WebServer::_running = true;

WebServer::WebServer(const std::string &proxyPass, int port)
    : _proxyPass(proxyPass)
{
    _serverSocket = createServerSocket(port);
    setServerSocketFlags(_serverSocket);
}

void WebServer::setServerSocketFlags(int socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0)
        return (WebErrors::printerror("fcntl(F_GETFL) failed"), void());
    flags |= O_NONBLOCK | O_CLOEXEC;
    if (fcntl(socket, F_SETFL, flags) < 0)
        return (WebErrors::printerror("fcntl(F_SETFL) failed"), void());
}

WebServer::~WebServer()
{
    close(_serverSocket);
}

int WebServer::createServerSocket(int port)
{
    int opt = 1;
    ScopedSocket serverSocket(socket(AF_INET, SOCK_STREAM, 0));

    if (serverSocket.get() < 0 || 
        setsockopt(serverSocket.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return (WebErrors::printerror("Error opening or configuring server socket"), -1);

    memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(port);

    if (bind(serverSocket.get(), (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
        return (WebErrors::printerror("Error binding server socket"), -1);

    if (listen(serverSocket.get(), 1000) < 0)
        return (WebErrors::printerror("Error listening on server socket"), -1);

    return serverSocket.release();
}


void WebServer::handleClient(int clientSocket)
{
    ScopedSocket    client(clientSocket);
    char            buffer[4096];
    ssize_t         bytesRead = recv(client.get(), buffer, sizeof(buffer), 0);
    addrinfo        hints{};
    addrinfo        *res = nullptr;

    if (bytesRead <= 0)
        throw WebErrors::ClientException("Error reading from client socket");

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("localhost", "8080", &hints, &res) != 0)
        throw WebErrors::ClientException("Error resolving proxy host");

    ScopedSocket proxySocket(socket(res->ai_family, res->ai_socktype, res->ai_protocol));

    if (proxySocket.get() < 0 || connect(proxySocket.get(), res->ai_addr, res->ai_addrlen) < 0)
        throw WebErrors::ClientException("Error connecting to proxy server", res);

    if (send(proxySocket.get(), buffer, bytesRead, 0) < 0)
        throw WebErrors::ClientException("Error sending to proxy server", res);

    while ((bytesRead = recv(proxySocket.get(), buffer, sizeof(buffer), 0)) > 0)
    {
        if (send(client.get(), buffer, bytesRead, 0) < 0)
            throw WebErrors::ClientException("Error sending to client socket", res);
    }
    if (bytesRead < 0)
        throw WebErrors::ClientException("Error reading from proxy server", res);

    freeaddrinfo(res);
}


void WebServer::start()
{
    std::cout << "Server is running. Press Ctrl+C to stop.\n";
    signal(SIGINT, [](int signum) { (void)signum; WebServer::_running = false; });

    while (_running)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
            {
                WebErrors::printerror("Error accepting connection");
                continue;
            }
        }
        try
        {
            handleClient(clientSocket);
        }
        catch (const WebErrors::ClientException &e)
        {
            WebErrors::printerror(e.what());
        }
    }

    std::cout << "Server stopped.\n";
}
