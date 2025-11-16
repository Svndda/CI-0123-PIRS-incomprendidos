#include "auth_udp_server.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <cstring>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include "../../../common/LogManager.h"
// Estructuras para mensajes DISCOVER 
#pragma pack(push, 1)
struct DiscoverRequest {
    uint8_t msgId;
    uint8_t flags;
};

struct DiscoverResponse {
    uint8_t msgId;
    uint8_t sensorId;
    uint8_t type;
    uint8_t status;
};
#pragma pack(pop)

AuthUDPServer::AuthUDPServer(const std::string& ip, uint16_t port)
    : UDPServer(ip, port, 1024) {
    loadDefaultUsers();
    std::cout << " AuthUDPServer iniciado en puerto UDP " << port << std::endl;
    auto& logger = LogManager::instance();
    
    // Enable file logging for security audit trail
    logger.enableFileLogging("auth_security_audit.log");
    
    logger.ipAddress(ip + std::string(":") + std::to_string(port));
    logger.info("AuthUDPServer initialized - LOCAL LOGGING ONLY (External Ring Security)");
    logger.info("Authentication server ready on " + ip + ":" + std::to_string(port) + 
                " - " + std::to_string(users.size()) + " users loaded");
}

AuthUDPServer::~AuthUDPServer() {
    auto& logger = LogManager::instance();
    logger.info("AuthUDPServer shutting down - Security logging ended");
    logger.disableFileLogging();
    stop();
}

void AuthUDPServer::loadDefaultUsers() {
    addUser("realAdmin", "M2sv8KxpLq", "system_admin", 7);
    addUser("audiTT", "gH5pxL9pQ", "auditor", 5);
    addUser("dataAdmin", "N7vbq2R0", "data_admin", 6);
    addUser("guestAA", "aB7nyZt9Qw1", "guest", 4);
    
    std::cout << " Usuarios cargados:" << std::endl;
    for (const auto& user : users) {
        std::cout << "   - " << user.first << " (" << user.second.getGroup() << ")" << std::endl;
    }
}

bool AuthUDPServer::addUser(const std::string& username, const std::string& password,
                           const std::string& group, int permissions) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (users.find(username) != users.end()) {
        return false;
    }
    
    std::string password_hash = User::hashSHA256(password);

    const User new_user(username, password_hash, group, permissions);
    
    users[username] = new_user;
    return true;
}

void AuthUDPServer::onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len,
                             std::string& out_response) {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    uint16_t peerPort = ntohs(peer.sin_port);
    
    std::cout << " AuthUDPServer: " << len << " bytes de " 
              << ipStr << ":" << peerPort << std::endl;
    
    if (len == sizeof(DiscoverRequest)) {
        handleDiscover(peer, data, len, out_response);
    } else if (len == 50) { // Tamaño de AuthRequest
        handleAuthRequest(peer, data, len, out_response);
    } else {
        std::cout << " Longitud no soportada: " << len << " bytes" << std::endl;
    }
}

void AuthUDPServer::handleDiscover(const sockaddr_in& peer, const uint8_t* data, ssize_t len, 
                                  std::string& out_response) {
    const DiscoverRequest* discover = reinterpret_cast<const DiscoverRequest*>(data);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    
    auto& logger = LogManager::instance();
    logger.info("DISCOVER request received - msgId: " + std::to_string(discover->msgId) + 
                " from " + std::string(ipStr) + ":" + std::to_string(ntohs(peer.sin_port)));
    
    std::cout << " DISCOVER - msgId: " << static_cast<int>(discover->msgId)
              << " desde " << ipStr << std::endl;
    
    DiscoverResponse response;
    response.msgId = discover->msgId;
    response.sensorId = 1;
    response.type = 1;
    response.status = 1;

    out_response.assign(reinterpret_cast<const char*>(&response), sizeof(response));
    
    logger.info("DISCOVER response sent to " + std::string(ipStr) + " - sensorId: 1");
    std::cout << " DISCOVER_RESP enviado" << std::endl;
}

void AuthUDPServer::handleAuthRequest(const sockaddr_in& peer, const uint8_t* data, ssize_t len,
                                     std::string& out_response) {
    // Crear AuthRequest desde el buffer recibido
    std::array<uint8_t, 50> buffer;
    std::memcpy(buffer.data(), data, 50);
    // Reconstruir AuthRequest manualmente desde el buffer
    uint16_t sessionId = (buffer[0] << 8) | buffer[1];
    
    std::array<char, AuthRequest::USERNAME_SIZE> username_array;
    std::memcpy(username_array.data(), buffer.data() + 2, AuthRequest::USERNAME_SIZE);
    std::string username(username_array.data(), strnlen(username_array.data(), AuthRequest::USERNAME_SIZE));
    
    std::array<char, AuthRequest::PASSWORD_SIZE> password_array;
    std::memcpy(password_array.data(), buffer.data() + 2 + AuthRequest::USERNAME_SIZE, AuthRequest::PASSWORD_SIZE);
    std::string password_hash(password_array.data(), strnlen(password_array.data(), AuthRequest::PASSWORD_SIZE));
    
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    
    auto& logger = LogManager::instance();
    std::string clientInfo = std::string(ipStr) + ":" + std::to_string(ntohs(peer.sin_port));
    
    logger.info("AUTH_REQUEST received - sessionId: " + std::to_string(sessionId) + 
                ", user: '" + username + "' from " + clientInfo);
    
    std::cout << " AUTH_REQUEST - sessionId: " << sessionId
              << ", usuario: '" << username << "'" << std::endl;

    // Procesar autenticación
    std::lock_guard<std::mutex> lock(users_mutex);
    
    AuthResponse response;
    response.setSessionId(sessionId);
    
    auto it = users.find(username);
    if (it != users.end() && !it->second.isLocked() &&
        it->second.verifyHashPassword(password_hash)) {
        
        response.setStatusCode(1);
        response.setMessage("Authentication successful");
        
        std::string sessionToken = generateSessionID();
        response.setSessionToken(sessionToken);
        
        // Registrar sesión activa
        {
            std::lock_guard<std::mutex> session_lock(sessions_mutex);
            active_sessions[sessionToken] = username;
        }
        
        // Log de seguridad - autenticación exitosa
        logger.info("AUTHENTICATION SUCCESS - User: '" + username + 
                   "', Group: '" + it->second.getGroup() + 
                   "', Client: " + clientInfo + 
                   ", SessionToken: " + sessionToken.substr(0, 8) + "...");
        
        std::cout << " Autenticación EXITOSA: " << username << std::endl;
        std::cout << "   Token de sesión: " << sessionToken << std::endl;
    } else {
        response.setStatusCode(0);
        response.setMessage("Invalid credentials");
        response.setSessionToken("");
        
        // Incrementar intentos fallidos si el usuario existe
        if (it != users.end()) {
            if (it->second.isLocked()) {
                logger.error("AUTHENTICATION BLOCKED - User: '" + username + 
                            "', Reason: Account locked, Client: " + clientInfo);
                std::cout << " Cuenta BLOQUEADA: " << username << std::endl;
            } else {
                logger.warning("AUTHENTICATION FAILED - User: '" + username + 
                              "', Reason: Invalid password, Client: " + clientInfo);
            }
        } else {
            logger.warning("AUTHENTICATION FAILED - User: '" + username + 
                          "', Reason: User not found, Client: " + clientInfo);
        }
        
        std::cout << " Autenticación FALLIDA: " << username << std::endl;
    }

    // Serializar AuthResponse a buffer
    auto response_buffer = response.toBuffer();
    out_response.assign(response_buffer.begin(), response_buffer.end());
    sendTo(peer, reinterpret_cast<const uint8_t*>(response_buffer.data()), response_buffer.size());

    logger.info("AUTH_RESPONSE sent to " + clientInfo + 
                " - status: " + std::to_string(response.getStatusCode()) + 
                " (" + (response.getStatusCode() == 1 ? "SUCCESS" : "FAILED") + ")");
    
    std::cout << " AUTH_RESPONSE enviado - status: " 
              << static_cast<int>(response.getStatusCode()) << std::endl;
}

std::string AuthUDPServer::generateSessionID() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1000000);
    
    std::stringstream ss;
    ss << millis << "_" << dis(gen);
    
    std::string data = ss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    std::stringstream hash_ss;
    for (int i = 0; i < 8; ++i) {
        hash_ss << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(hash[i]);
    }
    
    return hash_ss.str();
}