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

        if (fcntl(clientSocket, F_GETFD) == -1) {
            std::cerr << "Invalid file descriptor before epoll_ctl: " << strerror(errno) << std::endl;
             std::cerr << "Invalid file descriptor before epoll_ctl: " << strerror(errno) << std::endl;
              std::cerr << "Invalid file descriptor before epoll_ctl: " << strerror(errno) << std::endl;
            throw std::runtime_error("Invalid file descriptor");
        }




        if (epoll_ctl(_epollFd, operation, clientSocket, &event) == -1)
        {
            std::cout << "Error changing epoll state" <<  std::string(strerror(errno)) << std::endl;
             std::cout << "Error changing epoll state" <<  std::string(strerror(errno)) << std::endl;
              std::cout << "Error changing epoll state" <<  std::string(strerror(errno)) << std::endl;
               std::cout << "Error changing epoll state" <<  std::string(strerror(errno)) << std::endl;
                std::cout << "Error changing epoll state" <<  std::string(strerror(errno)) << std::endl;
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

void WebServer::acceptAddClientToEpoll(int clientSocketFd)
{
    try
    {
        struct sockaddr_in  clientAddr;
        socklen_t           clientLen = sizeof(clientAddr);
        ScopedSocket        clientSocket(accept(clientSocketFd, (struct sockaddr *)&clientAddr, &clientLen), 0);

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
            if (bytesRead > 0)
            {
                totalRequest.append(buffer, bytesRead);
                if (totalBytes == -1)
                    totalBytes = getRequestTotalLength(totalRequest);
            }
            else if (bytesRead == 0)
                throw std::runtime_error("Client closed connection");
            else if (bytesRead == -1)
                throw std::runtime_error("Error reading from client");
        }
        if (!totalRequest.empty())
        {
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
        auto cgiIt = std::find_if(_cgiProcessInfos.begin(), _cgiProcessInfos.end(),
                                  [clientSocket](const CgiInfo &cgiInfo) { return cgiInfo.clientSocket == clientSocket; });
        
        if (cgiIt != _cgiProcessInfos.end() && cgiIt->isDone)
        {
            const int bytesSent = send(clientSocket, cgiIt->responseBuffer.c_str(), cgiIt->responseBuffer.length(), 0);
            if (bytesSent == -1)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
                throw std::runtime_error("Error sending CGI response to client");
            }
            else
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
                _cgiProcessInfos.erase(cgiIt);
            }
        }
        else
        {
            auto it = _requestMap.find(clientSocket);
            if (it != _requestMap.end())
            {
                const Request &request = it->second;
                Response res(request, *this);

                const int bytesSent = send(clientSocket, res.getResponse().c_str(), res.getResponse().length(), 0);

                if (bytesSent == -1)
                {
                    epollController(clientSocket, EPOLL_CTL_DEL, 0);
                    throw std::runtime_error("Error sending response to client");
                }
                else
                    epollController(clientSocket, EPOLL_CTL_DEL, 0);
            }
            _requestMap.erase(it);
        }
    }
    catch (const std::exception &e)
    {
        try
        {
            epollController(clientSocket, EPOLL_CTL_DEL, 0);
        }
        catch (const std::exception &inner_e)
        {
            std::cout << "CATCHED INNER EXCEPTION" << std::endl;
            std::cout << "CATCHED INNER EXCEPTION OUT" << std::endl;
             std::cout << "CATCHED INNER EXCEPTION OUT" << std::endl;
              std::cout << "CATCHED INNER EXCEPTION OUT" << std::endl;
            WebErrors::combineExceptions(e, inner_e);
        }
        throw;
    }
}

void WebServer::handleCgiResponse(CgiInfo &cgiInfo)
{
    try
    {
        char buffer[1024];
        ssize_t bytesRead = read(cgiInfo.readEndFd, buffer, sizeof(buffer));
        if (bytesRead > 0)
        {
            cgiInfo.responseBuffer.append(buffer, bytesRead);
        }
        else if (bytesRead == 0)
        {
            cgiInfo.isDone = true;
            epollController(cgiInfo.clientSocket, EPOLL_CTL_MOD, EPOLLOUT);
        }
        else if (bytesRead == -1)
        {
            std::cerr << "Error reading from CGI process." << std::endl;
            throw std::runtime_error("Error reading from CGI process.");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in handleCgiResponse: " << e.what() << std::endl;
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
            _currentEventFd = _events[i].data.fd;

            if (getCorrectServerSocket(_currentEventFd))
            {
                acceptAddClientToEpoll(_currentEventFd);
            }
            else
            {
                // Check if the current FD is related to a CGI process
                auto cgiIt = std::find_if(_cgiProcessInfos.begin(), _cgiProcessInfos.end(),
                                          [this](const CgiInfo &cgiInfo) { return cgiInfo.readEndFd == _currentEventFd; });
                
                if (cgiIt != _cgiProcessInfos.end())
                {
                    // Handle CGI response data
                    handleCgiResponse(*cgiIt);
                }
                else
                {
                    if (_events[i].events & EPOLLIN)
                        handleIncomingData(_currentEventFd);
                    else if (_events[i].events & EPOLLOUT)
                        handleOutgoingData(_currentEventFd);
                }
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

std::vector<CgiInfo>  WebServer::getCgiInfos() { return _cgiProcessInfos; }


int WebServer::getCurrentEventFd() const { return _currentEventFd; }
