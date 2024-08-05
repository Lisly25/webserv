#include "RequestHandler.hpp"
#include "ScopedSocket.hpp"
#include "WebErrors.hpp"
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>

RequestHandler::RequestHandler(void) : _proxyInfo(nullptr)
{
    resolveProxyAddresses();
}

void RequestHandler::storeRequest(int clientSocket, const std::string &request)
{
    _requestMap[clientSocket] = request;
}

RequestHandler::~RequestHandler()
{
    //if (_proxySocket != -1)
     //   close(_proxySocket);
    freeaddrinfo(_proxyInfo);
    _proxyInfo = nullptr;
    
}

void RequestHandler::handleProxyPass(const std::string &request, std::string &response)
{
    // Send the request to the proxy server
    int proxySocket = socket(_proxyInfo->ai_family, _proxyInfo->ai_socktype, _proxyInfo->ai_protocol);
    if (proxySocket < 0 || connect(proxySocket, _proxyInfo->ai_addr, _proxyInfo->ai_addrlen) < 0)
        throw std::runtime_error("Error connecting to proxy server");

    std::cout << "Forwarding request to proxy:\n" << request << std::endl;

    if (send(proxySocket, request.c_str(), request.length(), 0) < 0)
        throw std::runtime_error("Error sending to proxy server");

    // Wait for the response from the proxy server
    char buffer[4096];
    ssize_t bytesRead = 0;
    while ((bytesRead = recv(proxySocket, buffer, sizeof(buffer), 0)) > 0)
    {
        response.append(buffer, bytesRead);
        std::cout << "Received response chunk from proxy, size: " << bytesRead << std::endl;
    }

    if (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        throw std::runtime_error("Error reading from proxy server");

    std::cout << "Full response from proxy:\n" << response << std::endl;

    close(proxySocket);
}


std::string RequestHandler::generateResponse(int clientSocket)
{
    auto it = _requestMap.find(clientSocket);
    if (it == _requestMap.end())
    {
        throw std::runtime_error("Request not found for client socket");
    }

    const std::string &request = it->second;
    std::string response;

    if (request.find("GET /proxy") != std::string::npos || request.find("GET / ") != std::string::npos)
    {
        handleProxyPass(request, response);
    }
    else
    {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, this is the response from the server.\r\n";
    }

    _requestMap.erase(it);

    return response;
}



// Find the locations from the config file
bool RequestHandler::parseRequest(const char* request, ssize_t length)
{

/**  CHECK THAT FROM WAHT LOCATION USER WANTS THE DATA AND DOES THE 
    *   DO WE PASS THE DATA TO CGI OR PROXY TO OTHER SERVER
    location /perse
    {
        proxy_pass http://localhost:8080;
        cgi_pass /usr/bin/php-cgi;
    }
 */

    (void )request;
    (void )length;
    return false;
}

void RequestHandler::handleCgiPass(const char* request, ssize_t length)
{
   (void )request;
    (void )length;
}

void RequestHandler::resolveProxyAddresses()
{
    const char* proxyHost = "localhost";
    const char* proxyPort = "8080";

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(proxyHost, proxyPort, &hints, &_proxyInfo);
    if (status != 0)
        throw WebErrors::ClientException("Error resolving proxy address: "\
            + std::string(gai_strerror(status)));
}
