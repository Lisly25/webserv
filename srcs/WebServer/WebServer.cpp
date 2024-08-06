#include "WebServer.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <csignal>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "Response.hpp"
#include "Request.hpp"

bool WebServer::_running = true;

WebServer::WebServer(WebParser &parser, int port)
    : _serverSocket(createServerSocket(port)), _epollFd(epoll_create(1)), _parser(parser)
{
    if (_epollFd == -1)
        throw WebErrors::ServerException("Error creating epoll instance");
    try{
        epollController(_serverSocket.get(), EPOLL_CTL_ADD, EPOLLIN | EPOLLOUT);
    }
    catch (const std::exception &e){
        throw ;
    }
}

WebServer::~WebServer() { close(_epollFd); }

int WebServer::createServerSocket(int port)
{
    int             opt = 1;
    ScopedSocket    serverSocket(socket(AF_INET, SOCK_STREAM, 0));

    if (serverSocket.get() < 0 || 
        setsockopt(serverSocket.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw WebErrors::ServerException("Error opening or configuring server socket");

    std::memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
    _serverAddr.sin_port = htons(port);
    if (bind(serverSocket.get(), (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0)
        throw WebErrors::ServerException("Error binding server socket");

    if (listen(serverSocket.get(), 1000) < 0)
        throw WebErrors::ServerException("Error listening on server socket");

    return serverSocket.release();
}

void WebServer::epollController(int clientSocket, int operation, uint32_t events)
{
    struct epoll_event  event;

    std::memset(&event, 0, sizeof(event));
    event.data.fd = clientSocket;
    event.events = events;
    if (epoll_ctl(_epollFd, operation, clientSocket, &event) == -1)
    {
        close(clientSocket);
        throw WebErrors::ServerException("Error changing epoll state" + std::string(strerror(errno)));
    }
    if (operation == EPOLL_CTL_DEL)
        close(clientSocket);
}

void WebServer::acceptAddClient()
{
    struct sockaddr_in  clientAddr;
    socklen_t           clientLen = sizeof(clientAddr);
    ScopedSocket        clientSocket(accept(_serverSocket.get(), (struct sockaddr *)&clientAddr, &clientLen));

    if (clientSocket.get() < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw WebErrors::ServerException("Error accepting client connection");

    epollController(clientSocket.get(), EPOLL_CTL_ADD, EPOLLIN);
    clientSocket.release();
}

std::string WebServer::getBoundary(const std::string &request)
{
    size_t start = request.find("boundary=");
    if (start == std::string::npos) return "";

    start += 9; // Length of "boundary="
    size_t end = request.find("\r\n", start);
    return (end == std::string::npos) ? request.substr(start) : request.substr(start, end - start);
}

int WebServer::getRequestTotalLength(const std::string &request)
{
    size_t contentLengthPos = request.find("Content-Length: ");
    if (contentLengthPos == std::string::npos) return request.length();

    contentLengthPos += 16; // Length of "Content-Length: "
    size_t end = request.find("\r\n", contentLengthPos);
    const int contentLength = std::stoi(request.substr(contentLengthPos, end - contentLengthPos));

    std::string boundary = getBoundary(request);
    if (!boundary.empty())
    {
        size_t boundaryPos = request.rfind("--" + boundary);
        if (boundaryPos != std::string::npos)
            return boundaryPos + boundary.length() + 4; // +4 for "--\r\n"
    }
    return contentLength;
}

void WebServer::handleIncomingData(int clientSocket)
{
    try {
        char        buffer[1024];
        std::string totalRequest;
        int         totalBytes = -1;
        int         bytesRead = 0;

        while (totalBytes == -1 || totalRequest.length() < static_cast<size_t>(totalBytes))
        {
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                if (bytesRead == 0 || (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                WebErrors::printerror("Error reading from client socket");
                break;
            }

            totalRequest.append(buffer, bytesRead);

            if (totalBytes == -1)
                totalBytes = getRequestTotalLength(totalRequest);
        }
        if (!totalRequest.empty())
        {
            _requestMap[clientSocket] = Request(totalRequest);
            epollController(clientSocket, EPOLL_CTL_MOD, EPOLLOUT);
        }
    }
    catch (const std::exception &e)
    {
        WebErrors::printerror(e.what());
        epollController(clientSocket, EPOLL_CTL_DEL, 0);
    }
}

void WebServer::handleOutgoingData(int clientSocket)
{
    try {
        auto it = _requestMap.find(clientSocket);
        if (it != _requestMap.end())
        {
            const Request &request = it->second;
            Response response;
            std::string responseContent = response.generate(request);
            int bytesSent = send(clientSocket, responseContent.c_str(), responseContent.length(), 0);
            if (bytesSent == -1)
                WebErrors::printerror("Error sending response to client");
            epollController(clientSocket, EPOLL_CTL_MOD, EPOLLIN);
            _requestMap.erase(it);
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        close(clientSocket);
    }
}

void WebServer::handleEvents(int eventCount)
{
    for (int i = 0; i < eventCount; ++i) {

        if (_events[i].data.fd == _serverSocket.get())
        {
            fcntl(_events[i].data.fd, F_SETFL, O_NONBLOCK, FD_CLOEXEC);
            acceptAddClient();
        }
        else if (_events[i].events & EPOLLIN)
            handleIncomingData(_events[i].data.fd);
        else if (_events[i].events & EPOLLOUT)
            handleOutgoingData(_events[i].data.fd);
    }
}

void WebServer::start()
{
    std::cout << "Server is running. Press Ctrl+C to stop.\n";
    std::signal(SIGINT, [](int signum) { (void)signum; WebServer::_running = false; });

    while (_running)
    {
        int eventCount = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
        if (eventCount == -1)
        {
            if (errno == EINTR) continue;
            WebErrors::printerror("Epoll wait error");
            continue;
        }
        handleEvents(eventCount);
    }
    std::cout << "Server stopped.\n";
}
