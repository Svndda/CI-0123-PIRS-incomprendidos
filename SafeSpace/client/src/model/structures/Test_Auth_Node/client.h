#ifndef CLIENT_H
#define CLIENT_H

#include "AuthenticationRequest.h"
#include "AuthenticationResponse.h"
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <openssl/sha.h>
#include <vector>

class Client {
private:
    int sockfd;
    struct sockaddr_in server_addr;
    std::string server_ip;
    int server_port;
    bool connected;
    
public:
    Client(const std::string& ip = "127.0.0.1", int port = 8080);
    ~Client();
    
    bool connectToServer();
    void disconnect();
    bool isConnected() const { return connected; }
    AuthenticationResponse sendAuthentication(const std::string& username, const std::string& password);
    
    // Configuración
    void setServerAddress(const std::string& ip, int port);
    
private:
    std::string hashPassword(const std::string& password);
    bool initializeSocket();
};

#endif