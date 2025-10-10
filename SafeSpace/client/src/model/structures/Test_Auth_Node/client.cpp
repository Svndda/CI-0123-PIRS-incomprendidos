#include "client.h"
#include <cstring>
#include <sstream>
#include <iomanip>

Client::Client(const std::string& ip, int port) 
    : server_ip(ip), server_port(port), sockfd(-1), connected(false) {
    initializeSocket();
}

Client::~Client() {
    disconnect();
}

bool Client::initializeSocket() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creando socket" << std::endl;
        return false;
    }
    
    // Configurar timeout de conexión
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Dirección IP inválida: " << server_ip << std::endl;
        return false;
    }
    
    return true;
}

void Client::setServerAddress(const std::string& ip, int port) {
    server_ip = ip;
    server_port = port;
    disconnect();
    initializeSocket();
}

bool Client::connectToServer() {
    if (sockfd < 0) {
        std::cerr << "Socket no válido" << std::endl;
        return false;
    }
    
    std::cout << "Conectando al servidor " << server_ip << ":" << server_port << "..." << std::endl;
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error conectando al servidor " << server_ip << ":" << server_port << std::endl;
        std::cerr << "Asegúrate de que:" << std::endl;
        std::cerr << "1. El servidor esté ejecutándose" << std::endl;
        std::cerr << "2. La IP y puerto sean correctos" << std::endl;
        std::cerr << "3. El firewall permita conexiones en el puerto " << server_port << std::endl;
        connected = false;
        return false;
    }
    
    std::cout << "✅ Conectado al servidor de autenticación en " 
              << server_ip << ":" << server_port << std::endl;
    connected = true;
    return true;
}

void Client::disconnect() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
    connected = false;
}

std::string Client::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), 
           password.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

AuthenticationResponse Client::sendAuthentication(const std::string& username, 
                                                 const std::string& password) {
    if (!connected) {
        std::cerr << "No conectado al servidor" << std::endl;
        return AuthenticationResponse(0, AuthenticationResponse::AUTH_FAILED, 
                                    AuthenticationResponse::LOGIN_FAILURE);
    }
    
    // Crear solicitud de autenticación
    std::string password_hash = hashPassword(password);
    AuthenticationRequest auth_request(1, username, password_hash);
    
    std::cout << "Enviando solicitud de autenticación:" << std::endl;
    std::cout << auth_request << std::endl;
    
    try {
        // Serializar mensaje
        std::stringstream message;
        message << "AUTH|" << username << "|" << password_hash;
        std::string message_str = message.str();
        
        // Enviar longitud del mensaje primero
        uint32_t msg_length = htonl(message_str.length());
        if (send(sockfd, &msg_length, sizeof(msg_length), 0) < 0) {
            throw std::runtime_error("Error enviando longitud del mensaje");
        }
        
        // Enviar mensaje
        if (send(sockfd, message_str.c_str(), message_str.length(), 0) < 0) {
            throw std::runtime_error("Error enviando mensaje");
        }
        
        // Recibir respuesta
        uint32_t response_length;
        if (recv(sockfd, &response_length, sizeof(response_length), 0) < 0) {
            throw std::runtime_error("Error recibiendo longitud de respuesta");
        }
        response_length = ntohl(response_length);
        
        if (response_length > 1024) {
            throw std::runtime_error("Longitud de respuesta demasiado grande");
        }
        
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(sockfd, buffer, response_length, 0);
        if (bytes_received < 0) {
            throw std::runtime_error("Error recibiendo respuesta");
        }
        
        std::string response_str(buffer, bytes_received);
        std::cout << "Respuesta recibida: " << response_str << std::endl;
        
        // Parsear respuesta
        AuthenticationResponse response;
        std::istringstream iss(response_str);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 3) {
            if (tokens[0] == "SUCCESS") {
                response.setMsgId(1);
                response.setDataType(AuthenticationResponse::AUTH_SUCCESS);
                response.setData(AuthenticationResponse::LOGIN_SUCCESS);
            } else {
                response.setMsgId(1);
                response.setDataType(AuthenticationResponse::AUTH_FAILED);
                response.setData(AuthenticationResponse::LOGIN_FAILURE);
            }
        }
        
        return response;
        
    } catch (const std::exception& e) {
        std::cerr << "Error en comunicación: " << e.what() << std::endl;
        connected = false;
        return AuthenticationResponse(0, AuthenticationResponse::AUTH_FAILED, 
                                    AuthenticationResponse::LOGIN_FAILURE);
    }
}