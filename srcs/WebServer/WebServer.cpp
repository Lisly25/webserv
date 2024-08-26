#include "WebServer.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <algorithm>
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

WebServer::WebServer(WebParser &parser)
    : _epollFd(-1), _parser(parser), _events(MAX_EVENTS)
{
    try
    {
        _serverSockets = createServerSockets(parser.getServers());
        resolveProxyAddresses(parser.getServers());
        _epollFd = epoll_create(1);
        if (_epollFd == -1)
            throw WebErrors::ServerException("Error creating epoll instance");
        for (const auto& serverSocket : _serverSockets)
            epollController(serverSocket.getFd(), EPOLL_CTL_ADD, EPOLLIN);
    }
    catch (const std::exception& e)
    {
        throw;
    }
}

WebServer::~WebServer()
{
    for (auto& entry : _proxyInfoMap)
    {
        if (entry.second)
        {
            freeaddrinfo(entry.second);
            entry.second = nullptr;
        }
    }
    if (_epollFd != -1)
    {
        close(_epollFd);
        _epollFd = -1;
    }
}

void WebServer::resolveProxyAddresses(const std::vector<Server>& server_confs)
{
    try
    {
        for (const auto& server : server_confs)
        {
            for (const auto& location : server.locations)
            {
                if (location.type == PROXY)
                {
                    std::string proxyHost;
                    std::string proxyPort;

                    size_t colonPos = location.target.rfind(':');
                    if (colonPos != std::string::npos)
                    {
                        proxyHost = location.target.substr(0, colonPos);
                        proxyPort = location.target.substr(colonPos + 1);
                    }
                    else
                    {
                        proxyHost = location.target;
                        proxyPort = std::to_string(server.port);
                    }

                    std::string key = proxyHost + ":" + proxyPort;
                    if (_proxyInfoMap.find(key) == _proxyInfoMap.end())
                    {
                        addrinfo hints{};
                        hints.ai_family = AF_UNSPEC;
                        hints.ai_socktype = SOCK_STREAM;

                        addrinfo* proxyInfo = nullptr;
                        int status = getaddrinfo(proxyHost.c_str(), proxyPort.c_str(), &hints, &proxyInfo);
                        if (status != 0)
                        {
                            throw WebErrors::ProxyException("Error resolving proxy address: " 
                                + std::string(gai_strerror(status)));
                        }
                        _proxyInfoMap[key] = proxyInfo;
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        throw;
    }
}

std::vector<ServerSocket> WebServer::createServerSockets(const std::vector<Server> &server_confs)
{
    try
    {
        std::vector<ServerSocket> serverSockets;

        for (const auto& server_conf : server_confs) 
        {
            ServerSocket serverSocket(server_conf, O_NONBLOCK | FD_CLOEXEC);
            serverSockets.push_back(std::move(serverSocket));
        }
        return serverSockets;
    }
    catch (const std::exception& e)
    {
        throw;
    }
}

void WebServer::epollController(int clientSocket, int operation, uint32_t events)
{
    try
    {
        struct epoll_event  event;

        std::memset(&event, 0, sizeof(event));
        event.data.fd = clientSocket;
        event.events = events;
        if (epoll_ctl(_epollFd, operation, clientSocket, &event) == -1)
        {
            close(clientSocket);
            throw std::runtime_error("Error changing epoll state" + std::string(strerror(errno)));
        }
        if (operation == EPOLL_CTL_DEL)
        {
            close(clientSocket);
            clientSocket = -1;
        }
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

void WebServer::acceptAddClient(int serverSocketFd)
{
    try
    {
        struct sockaddr_in  clientAddr;
        socklen_t           clientLen = sizeof(clientAddr);
        ScopedSocket        clientSocket(accept(serverSocketFd, (struct sockaddr *)&clientAddr, &clientLen), O_NONBLOCK | FD_CLOEXEC);

        if (clientSocket.getFd() < 0)
            throw std::runtime_error("Error accepting client connection");

        epollController(clientSocket.getFd(), EPOLL_CTL_ADD, EPOLLIN);
        clientSocket.release();
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

std::string WebServer::getBoundary(const std::string &request)
{
    try
    {
        size_t start = request.find("boundary=");
        if (start == std::string::npos) return "";

        start += 9; // Length of "boundary="
        size_t end = request.find("\r\n", start);
        return (end == std::string::npos) ? request.substr(start) : request.substr(start, end - start);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error parsing request boundary");
    }
}

int WebServer::getRequestTotalLength(const std::string &request)
{
    try
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
    catch (const std::exception &e)
    {
        throw std::runtime_error("Error parsing request content length");
    }
}

void WebServer::handleIncomingData(int clientSocket)
{
    try
    {
        char        buffer[1024];
        std::string totalRequest;
        int         totalBytes = -1;
        int         bytesRead = 0;

        while (totalBytes == -1 || totalRequest.length() < static_cast<size_t>(totalBytes))
        {
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
            {
                if (bytesRead == 0)
                    throw std::runtime_error("Client closed connection");
                throw std::runtime_error("Error reading request from client");
            }

            totalRequest.append(buffer, bytesRead);

            if (totalBytes == -1)
                totalBytes = getRequestTotalLength(totalRequest);
        }
        if (!totalRequest.empty())
        {
            std::cout << "RAW REQUEST -> " << totalRequest << std::endl;
            _requestMap[clientSocket] = Request(totalRequest, _parser.getServers(), _proxyInfoMap);
            epollController(clientSocket, EPOLL_CTL_MOD, EPOLLOUT);
        }
    }
    catch (const std::exception &e)
    {
        try {
            epollController(clientSocket, EPOLL_CTL_DEL, 0);
        } catch (const std::exception &inner_e) {
            WebErrors::combineExceptions(e, inner_e);
        }
        throw;
    }
}


void WebServer::handleOutgoingData(int clientSocket)
{
    try
    {
        auto it = _requestMap.find(clientSocket);
        if (it != _requestMap.end())
        {
            const Request &request = it->second;
            Response res(request);
            //std::cout << "Response -> " << res.getResponse() << std::endl;
            int bytesSent = send(clientSocket, res.getResponse().c_str(), res.getResponse().length(), 0);
            if (bytesSent == -1)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
                throw std::runtime_error("Error sending response to client");
            }
            else
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
            // std::cout << "Response sent to client " << res.getResponse() << std::endl;
        }
        _requestMap.erase(it);
    }
    catch (const std::exception &e)
    {
        try {
            epollController(clientSocket, EPOLL_CTL_DEL, 0);
        } catch (const std::exception &inner_e) {
            WebErrors::combineExceptions(e, inner_e);
        }
        throw;
    }
}


void WebServer::handleEvents(int eventCount)
{
    try
    {
        auto getCorrectServerSocket = [this](int fd) -> bool {
            return std::any_of(_serverSockets.begin(), _serverSockets.end(),
                               [fd](const ScopedSocket& socket) { return socket.getFd() == fd; });
        };

        for (int i = 0; i < eventCount; ++i)
        {
            const int fd = _events[i].data.fd;

            if (getCorrectServerSocket(fd))
            {
                fcntl(fd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
                acceptAddClient(fd);
            }
            else
            {
                if (_events[i].events & EPOLLIN)
                    handleIncomingData(fd);
                else if (_events[i].events & EPOLLOUT)
                    handleOutgoingData(fd);
            }
        }
    }
    catch (const std::exception &e)
    {
        throw;  
    }
}

void WebServer::start()
{
    std::cout << "Server is running. Press Ctrl+C to stop.\n";
    std::signal(SIGINT, [](int signum) { (void)signum; WebServer::_running = false; });

    while (_running)
    {
        try
        {
            int eventCount = epoll_wait(_epollFd, _events.data(), MAX_EVENTS, -1);
            if (eventCount == -1)
            {
                if (errno == EINTR) continue;
                throw std::runtime_error("Epoll wait error");
            }
            handleEvents(eventCount);
        }
        catch (const std::exception &e)
        {
            WebErrors::printerror(e.what());
        }
    }
    std::cout << "Server stopped.\n";
}
