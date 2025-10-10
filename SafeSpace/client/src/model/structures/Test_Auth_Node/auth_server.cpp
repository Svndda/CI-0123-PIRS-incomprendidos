#include "auth_server.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <thread>
#include <openssl/sha.h>
#include <chrono>
#include <random>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdexcept>

AuthServer::AuthServer(int port) : port(port), server_fd(-1), running(false) {
    loadDefaultUsers();
}

AuthServer::~AuthServer() {
    stop();
}

std::string AuthServer::getServerIP() const {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    
    // Obtener nombre del host
    gethostname(hostbuffer, sizeof(hostbuffer));
    
    // Obtener información del host
    host_entry = gethostbyname(hostbuffer);
    if (host_entry == nullptr) {
        return "127.0.0.1";
    }
    
    // Convertir a string IP
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    return std::string(IPbuffer);
}

// Función helper para calcular hash (igual que en el cliente)
std::string AuthServer::hashPassword(const std::string& password) {
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

void AuthServer::loadDefaultUsers() {
    // Usuarios basados en los roles del documento
    // Ahora usamos hash sin salt para coincidir con el cliente
    addUser("admin", "admin123", "system_admin", 7);      // rwx
    addUser("auditor", "auditor123", "auditor", 5);       // r-x  
    addUser("datamanager", "data123", "data_admin", 6);   // rw-
    addUser("guest", "guest123", "guest", 4);            // r--
    addUser("jhonny", "jhonny123", "system_admin", 7);    // Usuario adicional
    addUser("aaron", "aaron123", "auditor", 5);          // Usuario adicional
    
    std::cout << "Usuarios cargados con hash SHA-256:" << std::endl;
    for (const auto& user : users) {
        std::cout << "  - " << user.first << ": " << user.second.password_hash.substr(0, 16) << "..." << std::endl;
    }
}

bool AuthServer::addUser(const std::string& username, const std::string& password,
                        const std::string& group, int permissions) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    if (users.find(username) != users.end()) {
        return false;
    }
    
    // Calcular hash de la misma manera que el cliente (sin salt)
    std::string password_hash = hashPassword(password);
    
    User new_user;
    new_user.username = username;
    new_user.password_hash = password_hash;
    new_user.salt = "";  // No usamos salt para coincidir con el cliente
    new_user.group = group;
    new_user.permissions = permissions;
    new_user.failed_attempts = 0;
    new_user.is_locked = false;
    
    users[username] = new_user;
    std::cout << "Usuario agregado: " << username << " (" << group << ")" << std::endl;
    return true;
}

bool AuthServer::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error creando socket" << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error configurando socket" << std::endl;
        close(server_fd);
        return false;
    }
    
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Escuchar en todas las interfaces
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Error en bind en puerto " << port << std::endl;
        std::cerr << "Asegúrate de que el puerto esté disponible" << std::endl;
        close(server_fd);
        return false;
    }
    
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Error en listen" << std::endl;
        close(server_fd);
        return false;
    }
    
    running = true;
    
    std::cout << "==========================================" << std::endl;
    std::cout << " Servidor de Autenticación Iniciado" << std::endl;
    std::cout << " IP del servidor: " << getServerIP() << std::endl;
    std::cout << " Puerto: " << port << std::endl;
    std::cout << " Autenticación con hash SHA-256" << std::endl;
    std::cout << " Usuarios disponibles:" << std::endl;
    for (const auto& user : users) {
        std::cout << "   - " << user.first << " (" << user.second.group << ")" << std::endl;
    }
    std::cout << "==========================================" << std::endl;
    
    return true;
}

void AuthServer::stop() {
    running = false;
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
}

void AuthServer::run() {
    while (running) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Error aceptando conexión" << std::endl;
            }
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "🔗 Cliente conectado desde " << client_ip << std::endl;
        
        // Manejar cliente en hilo separado
        std::thread client_thread(&AuthServer::handleClient, this, client_socket, std::string(client_ip));
        client_thread.detach();
    }
}

void AuthServer::handleClient(int client_socket, const std::string& client_ip) {
    // Configurar timeout
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    try {
        // Recibir longitud del mensaje
        uint32_t msg_length;
        ssize_t bytes_read = recv(client_socket, &msg_length, sizeof(msg_length), 0);
        
        if (bytes_read != sizeof(msg_length)) {
            std::cerr << "Error recibiendo longitud del mensaje de " << client_ip << std::endl;
            close(client_socket);
            return;
        }
        
        msg_length = ntohl(msg_length);
        
        if (msg_length > 1024 || msg_length == 0) {
            std::cerr << "Longitud de mensaje inválida de " << client_ip << std::endl;
            close(client_socket);
            return;
        }
        
        // Recibir mensaje
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        bytes_read = recv(client_socket, buffer, msg_length, 0);
        
        if (bytes_read != msg_length) {
            std::cerr << "Error recibiendo mensaje completo de " << client_ip << std::endl;
            close(client_socket);
            return;
        }
        
        std::string message(buffer, msg_length);
        std::cout << " Mensaje recibido de " << client_ip << ": " << message << std::endl;
        
        // Procesar mensaje
        std::istringstream iss(message);
        std::string command, username, password_hash_received;
        std::getline(iss, command, '|');
        std::getline(iss, username, '|');
        std::getline(iss, password_hash_received, '|');
        
        std::string response_str;
        
        if (command == "AUTH") {
            AuthenticationResponse auth_response = processAuthentication(username, password_hash_received);
            
            if (auth_response.getDataType() == AuthenticationResponse::AUTH_SUCCESS) {
                response_str = "SUCCESS|" + username + "|Authentication successful";
                std::cout << " Autenticación exitosa para " << username << " desde " << client_ip << std::endl;
            } else {
                response_str = "FAILED|" + username + "|Invalid credentials";
                std::cout << " Autenticación fallida para " << username << " desde " << client_ip << std::endl;
            }
        } else {
            response_str = "ERROR|Unknown command";
        }
        
        // Enviar respuesta
        uint32_t response_length = htonl(response_str.length());
        send(client_socket, &response_length, sizeof(response_length), 0);
        send(client_socket, response_str.c_str(), response_str.length(), 0);
        
    } catch (const std::exception& e) {
        std::cerr << "Error manejando cliente " << client_ip << ": " << e.what() << std::endl;
    }
    
    close(client_socket);
}

AuthenticationResponse AuthServer::processAuthentication(const std::string& username, 
                                                       const std::string& password_hash_received) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    auto it = users.find(username);
    if (it == users.end()) {
        std::cout << "Autenticación fallida - Usuario no encontrado: " << username << std::endl;
        return AuthenticationResponse(1, AuthenticationResponse::AUTH_FAILED, 
                                    AuthenticationResponse::LOGIN_FAILURE);
    }
    
    User& user = it->second;
    
    if (user.is_locked) {
        std::cout << "Autenticación fallida - Cuenta bloqueada: " << username << std::endl;
        return AuthenticationResponse(1, AuthenticationResponse::AUTH_FAILED, 
                                    AuthenticationResponse::LOGIN_FAILURE);
    }
    
    // DEBUG: Mostrar información de depuración
    std::cout << "🔍 Comparando hashes para usuario: " << username << std::endl;
    std::cout << "   Hash recibido: " << password_hash_received << std::endl;
    std::cout << "   Hash almacenado: " << user.password_hash << std::endl;
    std::cout << "   Coinciden: " << (password_hash_received == user.password_hash ? "SÍ" : "NO") << std::endl;
    
    // Comparar el hash recibido con el hash almacenado
    if (password_hash_received != user.password_hash) {
        user.failed_attempts++;
        std::cout << "Autenticación fallida - Hash no coincide: " << username 
                  << " (intento " << user.failed_attempts << ")" << std::endl;
        
        if (user.failed_attempts >= 3) {
            user.is_locked = true;
            std::cout << " Cuenta bloqueada por intentos fallidos: " << username << std::endl;
        }
        
        return AuthenticationResponse(1, AuthenticationResponse::AUTH_FAILED, 
                                    AuthenticationResponse::LOGIN_FAILURE);
    }
    
    // Autenticación exitosa
    user.failed_attempts = 0;
    std::string session_id = generateSessionID();
    
    {
        std::lock_guard<std::mutex> session_lock(sessions_mutex);
        active_sessions[session_id] = username;
    }
    
    std::cout << " Autenticación exitosa: " << username 
              << " (grupo: " << user.group 
              << ", permisos: " << user.permissions << ")" << std::endl;
    std::cout << "   Session ID: " << session_id << std::endl;
    
    return AuthenticationResponse(1, AuthenticationResponse::AUTH_SUCCESS, 
                                AuthenticationResponse::LOGIN_SUCCESS);
}

std::string AuthServer::generateSessionID() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1000000);
    
    std::stringstream ss;
    ss << millis << "_" << dis(gen);
    
    // Hash del ID de sesión
    std::string data = ss.str();
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    
    std::stringstream hash_ss;
    for (int i = 0; i < 8; ++i) { // Solo primeros 8 bytes para simplificar
        hash_ss << std::hex << std::setw(2) << std::setfill('0') 
               << static_cast<int>(hash[i]);
    }
    
    return hash_ss.str();
}