#pragma once

#include <string>
#include <netinet/in.h>

class WebServer {
public:
    WebServer(const std::string &proxyPass, int port);
    ~WebServer();
    void start();

private:
    int createServerSocket(int port);
    void handleClient(int clientSocket);

    std::string _proxyPass;
    int _serverSocket;
    struct sockaddr_in _serverAddr;
};
