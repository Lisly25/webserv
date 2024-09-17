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

volatile sig_atomic_t WebServer::s_serverRunning = 1;

WebServer::WebServer(WebParser &parser)
    : _epollFd(-1), _parser(parser), _events(MAX_EVENTS)
{
    try
    {
        std::cout << COLOR_GREEN_SERVER << "[ SERVER STARTED ] press Ctrl+C to stop 🏭 \n\n" << COLOR_RESET;
        _serverSockets = createServerSockets(parser.getServers());
        resolveProxyAddresses(parser.getServers());
        _epollFd = epoll_create(1);
        if (_epollFd == -1)
            throw WebErrors::ServerException("Error creating epoll");
        for (const auto& serverSocket : _serverSockets)
            epollController(serverSocket.getFd(), EPOLL_CTL_ADD, EPOLLIN, FdType::SERVER);
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

void WebServer::epollController(int clientSocket, int operation, uint32_t events, FdType fdType)
{
    try
    {
        struct epoll_event event;

        std::memset(&event, 0, sizeof(event));
        event.data.fd = clientSocket;
        event.events = events;

        if (EPOLL_CTL_ADD)
        {
            switch (fdType)
            {
                case FdType::SERVER:
                    std::cout << COLOR_GREEN_SERVER << " { Server socket added to epoll 🏊 }\n\n" << COLOR_RESET;
                    break;
                case FdType::CLIENT:
                    std::cout << COLOR_GREEN_SERVER << " { Client socket added to epoll 🏊 }\n\n" << COLOR_RESET;
                    break;
                case FdType::CGI_PIPE:
                    std::cout << COLOR_GREEN_SERVER << " { CGI pipe added to epoll 🏊 }\n\n" << COLOR_RESET;
                    break;
            }
        }
        if (epoll_ctl(_epollFd, operation, clientSocket, &event) == -1)
        {
            close(clientSocket);
            throw std::runtime_error("Error changing epoll state: " + std::string(strerror(errno)));
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

        setFdNonBlocking(clientSocket.getFd());
        epollController(clientSocket.getFd(), EPOLL_CTL_ADD, EPOLLIN, FdType::CLIENT);
        clientSocket.release();
    }
    catch (const std::exception &e)
    {
        throw;
    }
}

void WebServer::handleIncomingData(int clientSocket)
{
    auto cleanupClient = [this](int clientSocket)
    {
        std::cout << "Cleaning up client socket: " << clientSocket << "\n";
        epollController(clientSocket, EPOLL_CTL_DEL, 0, FdType::CLIENT);
        _partialRequests.erase(clientSocket);
    };

    auto isRequestComplete = [](const std::string &request) -> bool
    {
        size_t              headerEnd = request.find("\r\n\r\n");

        if (headerEnd == std::string::npos)
            return false;

        size_t              contentLength = 0;
        std::istringstream  stream(request.substr(0, headerEnd + 4));
        std::string         line;

        while (std::getline(stream, line) && line != "\r")
        {
            if (line.find("Content-Length:") != std::string::npos)
            {
                contentLength = std::stoul(line.substr(15));
                break;
            }
        }

        const size_t totalLength = headerEnd + 4 + contentLength;
        return request.length() >= totalLength;
    };

    auto extractCompleteRequest = [](const std::string &buffer) -> std::string
    {
        size_t              headerEnd = buffer.find("\r\n\r\n");

        if (headerEnd == std::string::npos)
            return "";
        size_t              contentLength = 0;
        std::istringstream  stream(buffer.substr(0, headerEnd + 4));
        std::string         line;

        while (std::getline(stream, line) && line != "\r")
        {
            if (line.find("Content-Length:") != std::string::npos)
            {
                contentLength = std::stoul(line.substr(15));
                break;
            }
        }
        const size_t        totalLength = headerEnd + 4 + contentLength;
        return buffer.substr(0, totalLength);
    };

    auto processRequest = [this](int clientSocket, const std::string &requestStr)
    {
        Request request(requestStr, _parser.getServers(), _proxyInfoMap);
        
        _requestMap[clientSocket] = request;

        std::cout << COLOR_MAGENTA_SERVER << "  Request to: " << request.getServer()->server_name[0] << \
            ":" << request.getServer()->port <<  request.getRequestData().originalUri << " ✉️\n\n" << COLOR_RESET;
        if (request.getLocation()->type == LocationType::CGI && request.getErrorCode() == 0)
        {
            CGIHandler cgiHandler(request, *this);
            epoll_ctl(_epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
        }
        else
            epollController(clientSocket, EPOLL_CTL_MOD, EPOLLOUT, FdType::CLIENT);
    };

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
            std::cout << "Client disconnected, closing socket: " << clientSocket << "\n";
            cleanupClient(clientSocket);
        }
        else if (bytesRead == -1)
        {
            std::cerr << "Error reading data from client socket: " << strerror(errno) << "\n";
            cleanupClient(clientSocket);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleIncomingData: " << e.what() << "\n";
        cleanupClient(clientSocket);
    }
}

void WebServer::handleOutgoingData(int clientSocket)
{
    try
    {
        {
            const Request &request = _requestMap[clientSocket];
            Response res(request);

            const int bytesSent = send(clientSocket, res.getResponse().c_str(), res.getResponse().length(), 0);

            if (bytesSent == -1)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0, FdType::CLIENT);
                throw std::runtime_error("Error sending response to client");
            }
            else if (bytesSent == 0)
            {
                epollController(clientSocket, EPOLL_CTL_DEL, 0, FdType::CLIENT);
                throw std::runtime_error("Connection closed by the client");
            }
            else
                epollController(clientSocket, EPOLL_CTL_DEL, 0, FdType::CLIENT);
        }
    }
    catch (const std::exception &e)
    {
        try {
            epollController(clientSocket, EPOLL_CTL_DEL, 0, FdType::CLIENT);
        } catch (const std::exception &inner_e) {
            WebErrors::combineExceptions(e, inner_e);
        }
        throw;
    }
}

void WebServer::handleCgiInteraction(std::list<CGIProcessInfo>::iterator it, int pipeFd, uint32_t events)
{
    auto handleRead = [this](auto it, int fromCgiFd)
    {
        char          buffer[4096];
        const ssize_t bytes = read(fromCgiFd, buffer, sizeof(buffer));

        if (bytes > 0)
            it->response.append(buffer, bytes);
        if (bytes != -1)
        {
            const int clientSocket = it->clientSocket;
            epollController(fromCgiFd, EPOLL_CTL_DEL, 0, FdType::CGI_PIPE);  
            ssize_t sent = send(clientSocket, it->response.c_str(), it->response.size(), 0);
            if (sent == -1)
                std::cerr << COLOR_RED_ERROR << "Error sending CGI response to client: " << strerror(errno) << "\n\n" << COLOR_RESET;
            close(clientSocket);
            _cgiInfoList.erase(it);
        }
        else if (bytes == -1)
            throw std::runtime_error("Error reading from CGI output pipe");
    };

    auto handleWrite = [this](auto it, int toCgiFd)
    {
        CGIProcessInfo& cgiInfo = *it;
        const char*     dataToWrite = cgiInfo.pendingWriteData.c_str() + cgiInfo.writeOffset;
        const size_t    remainingData = cgiInfo.pendingWriteData.size() - cgiInfo.writeOffset;
        const ssize_t   written = write(toCgiFd, dataToWrite, remainingData);

        if (written == -1)
            throw std::runtime_error("Error writing to CGI input pipe");

        cgiInfo.writeOffset += written;
        if (cgiInfo.writeOffset == cgiInfo.pendingWriteData.size())
        {
            epollController(toCgiFd, EPOLL_CTL_DEL, 0, FdType::CGI_PIPE);
        }
    };

    if (events & EPOLLIN && it->readFromCgiFd == pipeFd)
    {
        handleRead(it, pipeFd);
    }
    if (events & EPOLLOUT && it->writeToCgiFd == pipeFd)
    {
        handleWrite(it, pipeFd);
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

        auto findCgiEvent = [this](int fd) -> std::list<CGIProcessInfo>::iterator {
            return std::find_if(_cgiInfoList.begin(), _cgiInfoList.end(),
                                [fd](const CGIProcessInfo& cgiInfo) {
                                    return cgiInfo.readFromCgiFd == fd || cgiInfo.writeToCgiFd == fd;
                                });
        };

        for (int i = 0; i < eventCount; ++i)
        {
            _currentEventFd = _events[i].data.fd;
            if (getCorrectServerSocket(_currentEventFd))
            {
                acceptAddClientToEpoll(_currentEventFd);
                continue;
            }
            auto cgiInfoIter = findCgiEvent(_currentEventFd);
            if (cgiInfoIter != _cgiInfoList.end())
            {
                handleCgiInteraction(cgiInfoIter, _currentEventFd, _events[i].events);
                continue;
            }
            if (_events[i].events & EPOLLIN)
                handleIncomingData(_currentEventFd);
            else if (_events[i].events & EPOLLOUT)
                handleOutgoingData(_currentEventFd);
        }
    }
    catch (const std::exception &e)
    {
        throw;
    }
}


void WebServer::CGITimeoutChecker(void)
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = _cgiInfoList.begin(); it != _cgiInfoList.end();)
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->startTime).count();

        if (elapsed > CGI_TIMEOUT_LIMIT)
        {
            std::cout << COLOR_YELLOW_CGI << "  CGI Script Timed Out ⏰\n\n" << COLOR_RESET;
            if (kill(it->pid, SIGKILL) == -1)
                std::cerr << COLOR_RED_ERROR << "Failed to kill CGI process: " << strerror(errno) << "\n\n" << COLOR_RESET;
            if (it->clientSocket >= 0 && fcntl(it->clientSocket, F_GETFD) != -1)
            {  
                std::string response;
                ErrorHandler(_requestMap[it->clientSocket]).handleError(response, 504);
                send(it->clientSocket, response.c_str(), response.length(), 0);
                close(it->clientSocket);
            }
            if (_requestMap[it->clientSocket].getRequestData().method == "POST")
                epollController(it->writeToCgiFd, EPOLL_CTL_DEL, 0, FdType::CGI_PIPE);
            epollController(it->readFromCgiFd, EPOLL_CTL_DEL, 0, FdType::CGI_PIPE);
            _requestMap.erase(it->clientSocket);
            it = _cgiInfoList.erase(it);
        }
        else
            ++it;
    }
}

void WebServer::start()
{
    std::signal(SIGINT, signalHandler);  
    std::signal(SIGTERM, signalHandler);
    std::signal(SIGQUIT, signalHandler);
    std::signal(SIGTSTP, signalHandler); 
    std::signal(SIGPIPE, SIG_IGN);

    while (s_serverRunning)
    {
        try
        {
            int eventCount = epoll_wait(_epollFd, _events.data(), MAX_EVENTS, 500);
            if (eventCount == -1)
            {
                if (errno == EINTR) continue;
                throw std::runtime_error("Epoll wait error");
            }
            if (eventCount > 0)
                handleEvents(eventCount);
            CGITimeoutChecker();
        }
        catch (const std::exception &e)
        {
            WebErrors::printerror("WebServer::start", e.what());
        }
    }
    std::cout << COLOR_GREEN_SERVER << "[ SERVER STOPPED ] 🔌\n" << COLOR_RESET;
}

void  WebServer::signalHandler(int signal) { (void) signal; s_serverRunning = 0; }

int WebServer::getEpollFd() const { return _epollFd; }

cgiInfoList& WebServer::getCgiInfoList() { return _cgiInfoList; }

int WebServer::getCurrentEventFd() const { return _currentEventFd; }

void WebServer::setFdNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        throw std::runtime_error("Failed to get fd flags");
    flags |= O_NONBLOCK | FD_CLOEXEC;
    if (fcntl(fd, F_SETFL, flags) == -1)
        throw std::runtime_error("Failed to set non-blocking mode");
}
