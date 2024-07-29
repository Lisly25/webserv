#include "WebServer.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

WebServer::WebServer(const std::string &proxyPass, int port)
    : _proxyPass(proxyPass) {
    _serverSocket = createServerSocket(port);
}

WebServer::~WebServer() {
    close(_serverSocket);
}

int WebServer::createServerSocket(int port)
{
    // Create ipv4, steam socket, protocol tcp
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    // Allow the socket to be reused immediately after closing
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind the socket to the port
    memset(&_serverAddr, 0, sizeof(_serverAddr));
    _serverAddr.sin_family = AF_INET; // ipv4
    _serverAddr.sin_addr.s_addr = INADDR_ANY; // bind all interfaces available
    _serverAddr.sin_port = htons(port); // convert to network byte order

    if (bind(serverSocket, (struct sockaddr *)&_serverAddr, sizeof(_serverAddr)) < 0) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 1000) < 0) {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }

    return serverSocket;
}

void WebServer::handleClient(int clientSocket) {
    char buffer[4096];
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
    if (bytesRead <= 0) {
        std::cerr << "Error reading from client socket: " << strerror(errno) << std::endl;
        close(clientSocket);
        return;
    }

    addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get the info from running container from localhost:8080
    if (getaddrinfo("localhost", "8080", &hints, &res) != 0) {
        std::cerr << "Error resolving proxy host" << std::endl;
        return (close(clientSocket), void());
        return;
    }

    // Open a socket to the container for the data transfer
    int proxySocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (proxySocket < 0 || connect(proxySocket, res->ai_addr, res->ai_addrlen) < 0) {
        std::cerr << "Error connecting to proxy server" << std::endl;
        close(proxySocket);
        freeaddrinfo(res);
        close(clientSocket);
        return;
    }

    // Forward the data from the client to the container
    write(proxySocket, buffer, bytesRead);

    // Forward the data from the container to the client
    while ((bytesRead = read(proxySocket, buffer, sizeof(buffer))) > 0)
        write(clientSocket, buffer, bytesRead);


    close(proxySocket);
    close(clientSocket);
    freeaddrinfo(res);
}
void WebServer::start() {
    std::cout << "Server is running. Press Ctrl+C to stop.\n";

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket < 0) {
            perror("Error accepting connection");
            continue;
        }

        handleClient(clientSocket);
    }
}
