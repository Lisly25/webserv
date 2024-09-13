#include "WebServer.hpp"
#include "CGIHandler.hpp"
#include "ErrorHandler.hpp"
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
        std::cout << COLOR_GREEN_SERVER << "[ SERVER STARTED ] press Ctrl+C to stop\n\n" << COLOR_RESET;
        _serverSockets = createServerSockets(parser.getServers());
        resolveProxyAddresses(parser.getServers());
        _epollFd = epoll_create(1);
        if (_epollFd == -1)
            throw WebErrors::ServerException("Error creating epoll");
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
                            throw WebErrors::ProxyException( "Error resolving proxy address" );
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
        struct epoll_event event;

        std::memset(&event, 0, sizeof(event));
        event.data.fd = clientSocket;
        event.events = events;

        if (epoll_ctl(_epollFd, operation, clientSocket, &event) == -1)
        {
            close(clientSocket);
            throw std::runtime_error("Error changing epoll state: " + std::string(strerror(errno)));
        }
        if (events & EPOLLIN)
        {
            std::cout << COLOR_GREEN_SERVER << "Socket: added to epoll EPOLLIN: " << COLOR_RESET << std::endl;
        }
        if (events & EPOLL_CTL_DEL)
        {
            std::cout << COLOR_YELLOW_CGI << "Socket: removed from epoll EPOLL_CTL_DEL: " << COLOR_RESET << std::endl;
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
            throw std::runtime_error( "Error accepting client" );

        epollController(clientSocket.getFd(), EPOLL_CTL_ADD, EPOLLIN);
        clientSocket.release();
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

std::string WebServer::extractCompleteRequest(const std::string &buffer)
{
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return ""; // Should not happen if isRequestComplete returned true

    size_t contentLength = 0;
    std::istringstream stream(buffer.substr(0, headerEnd + 4));
    std::string line;
    while (std::getline(stream, line) && line != "\r")
    {
        if (line.find("Content-Length:") != std::string::npos)
        {
            contentLength = std::stoul(line.substr(15));
            break;
        }
    }
    size_t totalLength = headerEnd + 4 + contentLength;
    return buffer.substr(0, totalLength);
}

bool WebServer::isRequestComplete(const std::string &request)
{
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;

    size_t contentLength = 0;
    std::istringstream stream(request.substr(0, headerEnd + 4));
    std::string line;
    while (std::getline(stream, line) && line != "\r")
    {
        if (line.find("Content-Length:") != std::string::npos)
        {
            contentLength = std::stoul(line.substr(15));
            break;
        }
    }

    size_t totalLength = headerEnd + 4 + contentLength;
    return request.length() >= totalLength;
}

void WebServer::cleanupClient(int clientSocket)
{
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
    _partialRequests.erase(clientSocket);
    _requestMap.erase(clientSocket);
    close(clientSocket);
}


void WebServer::processRequest(int clientSocket, const std::string &requestStr)
{
    std::cout << COLOR_CYAN_COOKIE << "Request: " << requestStr << COLOR_RESET << std::endl;
    Request request(requestStr, _parser.getServers(), _proxyInfoMap);
    _requestMap[clientSocket] = request;

    if (request.getLocation()->type == LocationType::CGI)
    {
        if (request.getErrorCode() != 0)
        {
            std::string response;
            ErrorHandler(request).handleError(response, request.getErrorCode());
            send(clientSocket, response.c_str(), response.length(), 0);
            cleanupClient(clientSocket);
        }
        else
        {
            std::cout << COLOR_YELLOW_CGI << "CGI request: " << request.getRequestData().uri << COLOR_RESET << std::endl;
            CGIHandler cgiHandler(request, *this);
        }
    }
    else
    {
        epollController(clientSocket, EPOLL_CTL_MOD, EPOLLOUT);
    }
}


void WebServer::handleIncomingData(int clientSocket)
{
    try
    {
        char buffer[1024];
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesRead > 0)
        {
            _partialRequests[clientSocket].append(buffer, bytesRead);

            while (isRequestComplete(_partialRequests[clientSocket]))
            {
                std::string completeRequest = extractCompleteRequest(_partialRequests[clientSocket]);

                _partialRequests[clientSocket].erase(0, completeRequest.length());

                processRequest(clientSocket, completeRequest);
            }
        }
        else if (bytesRead == 0)
        {
            cleanupClient(clientSocket);
        }
        else if (bytesRead == -1)
        {
            cleanupClient(clientSocket);
            throw std::runtime_error("Error receiving data from client");
        }
    }
    catch (const std::exception &e)
    {
        cleanupClient(clientSocket);
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

            const int bytesSent = send(clientSocket, res.getResponse().c_str(), res.getResponse().length(), 0);

            if (bytesSent == -1)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
                throw std::runtime_error("Error sending response to client");
            }
            else if (bytesSent == 0)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
                throw std::runtime_error("Connection closed by the client");
            }
            else
                epollController(clientSocket, EPOLL_CTL_DEL, 0);
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

void WebServer::handleCGIOutput(int pipeFd)
{
    auto it = _cgiInfoMap.find(pipeFd);
    if (it != _cgiInfoMap.end())
    {
        CGIProcessInfo &cgiInfo = it->second;

        char buffer[4096];
        ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

        if (bytes > 0)
        {
            std::cout << COLOR_YELLOW_CGI << "CGI output: " << std::string(buffer, bytes) << COLOR_RESET << std::endl;
            cgiInfo.response.append(buffer, bytes);
        }
        else if (bytes == 0)
        {
            epollController(pipeFd, EPOLL_CTL_DEL, 0);
            send(cgiInfo.clientSocket, cgiInfo.response.c_str(), cgiInfo.response.length(), 0);
            close(cgiInfo.clientSocket);
            _cgiInfoMap.erase(it);
        }
        else if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            throw std::runtime_error("Error reading from CGI output pipe");
        }
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
            uint32_t events = _events[i].events;
            _currentEventFd = fd;
            std::cout << "handleEvents: fd=" << fd << ", events=" << events << std::endl;
            for (const auto& entry : _cgiInfoMap)
            {
                std::cout << entry.first << " ";
            }
            if (getCorrectServerSocket(fd))
            {
                acceptAddClientToEpoll(fd);
            }
            else if (_cgiInfoMap.find(fd) != _cgiInfoMap.end())
            {
                handleCGIOutput(fd);
            }
            else
            {
                if (_events[i].events & EPOLLIN)
                {
                    handleIncomingData(fd);
                }
                else if (_events[i].events & EPOLLOUT)
                {
                    handleOutgoingData(fd);
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
            WebErrors::printerror("WebServer::start", e.what());
        }
    }
    std::cout << COLOR_GREEN_SERVER << "[ SERVER STOPPED ] \n" << COLOR_RESET;
}

int WebServer::getEpollFd() const
{
    return _epollFd;
}

std::unordered_map<int, CGIProcessInfo>& WebServer::getCgiInfoMap()
{
    return _cgiInfoMap;
}

int WebServer::getCurrentEventFd() const
{
    return _currentEventFd;
}
