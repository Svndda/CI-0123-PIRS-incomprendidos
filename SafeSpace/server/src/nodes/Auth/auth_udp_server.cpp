#include "auth_udp_server.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <cstring>
#include <openssl/sha.h>
#include <arpa/inet.h>
#include <regex>

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

AuthUDPServer::AuthUDPServer(const std::string& ip, uint16_t port)
    : UDPServer(ip, port, 1024) {
    
    // Mostrar formato de contraseña requerido al iniciar
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "NODO DE AUTENTICACIÓN - FORMATO DE CONTRASEÑAS" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << getPasswordFormatDescription() << std::endl;
    std::cout << std::string(50, '=') << std::endl << std::endl;
    
    loadDefaultUsers();
    std::cout << " AuthUDPServer iniciado en puerto UDP " << port << std::endl;
}

AuthUDPServer::~AuthUDPServer() {
    stop();
}

// Método para obtener descripción del formato de contraseña
std::string AuthUDPServer::getPasswordFormatDescription() {
    std::stringstream ss;
    ss << "La contraseña debe cumplir con los siguientes requisitos:\n";
    ss << "1. Mínimo 8 caracteres\n";
    ss << "2. Al menos una letra mayúscula\n";
    ss << "3. Al menos un número (0-9)\n";
    ss << "4. Al menos un símbolo (!@#$%^&*()_+-=[]{}|;:,.<>?)\n";
    ss << "5. Puede incluir letras minúsculas\n";
    ss << "\nEjemplos válidos: P@ssw0rd!, Secur3#123, MyP@ss123";
    return ss.str();
}

// Método para validar formato de contraseña
bool AuthUDPServer::validatePasswordFormat(const std::string& password) {
    // Verificar longitud mínima
    if (password.length() < 8) {
        std::cout << "   La contraseña debe tener al menos 8 caracteres" << std::endl;
        return false;
    }
    
    // Verificar al menos una mayúscula
    bool hasUpperCase = false;
    bool hasLowerCase = false;
    bool hasDigit = false;
    bool hasSymbol = false;
    
    // Definir símbolos permitidos
    const std::string symbols = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    for (char c : password) {
        if (std::isupper(c)) hasUpperCase = true;
        else if (std::islower(c)) hasLowerCase = true;
        else if (std::isdigit(c)) hasDigit = true;
        else if (symbols.find(c) != std::string::npos) hasSymbol = true;
    }
    
    // Verificar todos los requisitos
    if (!hasUpperCase) {
        std::cout << "   La contraseña debe contener al menos una letra mayúscula" << std::endl;
    }
    if (!hasDigit) {
        std::cout << "   La contraseña debe contener al menos un número" << std::endl;
    }
    if (!hasSymbol) {
        std::cout << "   La contraseña debe contener al menos un símbolo" << std::endl;
    }
    
    return hasUpperCase && hasDigit && hasSymbol;
}

void AuthUDPServer::loadDefaultUsers() {
    std::cout << "Cargando usuarios por defecto..." << std::endl;
    
    // Contraseñas que cumplen con el formato requerido
    std::vector<std::tuple<std::string, std::string, std::string, int>> defaultUsers = {
        {"realAdmin", "M2sv8KxpLq!", "system_admin", 7},      // 10 chars, mayúscula M, número 2, símbolo !
        {"audiTT", "gH5pxL9pQ@", "auditor", 5},              // 9 chars, mayúscula H y Q, número 5 y 9, símbolo @
        {"dataAdmin", "N7vbq2R0#", "data_admin", 6},         // 8 chars, mayúscula N y R, número 7,2,0, símbolo #
        {"guestAA", "aB7nyZt9Qw1$", "guest", 4}             // 11 chars, mayúscula B y Z, número 7,9,1, símbolo $
    };
    
    int loadedCount = 0;
    int failedCount = 0;
    
    for (const auto& user : defaultUsers) {
        const auto& [username, password, group, permissions] = user;
        
        std::cout << "\nVerificando usuario: " << username << std::endl;
        std::cout << "Contraseña: " << password << std::endl;
        
        if (validatePasswordFormat(password)) {
            if (addUser(username, password, group, permissions)) {
                std::cout << "   Usuario '" << username << "' cargado exitosamente" << std::endl;
                loadedCount++;
            } else {
                std::cout << "   Error al cargar usuario '" << username << "'" << std::endl;
                failedCount++;
            }
        } else {
            std::cout << "   Contraseña no cumple formato para usuario '" << username << "'" << std::endl;
            failedCount++;
        }
    }
    
    std::cout << "\nResumen de carga de usuarios:" << std::endl;
    std::cout << "  Cargados exitosamente: " << loadedCount << std::endl;
    std::cout << "  Fallidos: " << failedCount << std::endl;
    
    if (loadedCount > 0) {
        std::cout << "\nUsuarios disponibles:" << std::endl;
        std::lock_guard<std::mutex> lock(users_mutex);
        for (const auto& user : users) {
            std::cout << "  - " << user.first << " (" << user.second.getGroup() << ")" << std::endl;
        }
    }
}

bool AuthUDPServer::addUser(const std::string& username, const std::string& password,
                           const std::string& group, int permissions) {
    std::lock_guard<std::mutex> lock(users_mutex);
    
    // Verificar si el usuario ya existe
    if (users.find(username) != users.end()) {
        std::cout << "   El usuario '" << username << "' ya existe" << std::endl;
        return false;
    }
    
    // Validar formato de contraseña
    std::cout << "\nValidando formato de contraseña para '" << username << "'..." << std::endl;
    if (!validatePasswordFormat(password)) {
        std::cout << "   Formato de contraseña inválido para usuario '" << username << "'" << std::endl;
        return false;
    }
    
    // Generar hash de la contraseña
    std::string password_hash = User::hashSHA256(password);

    // Crear y almacenar usuario
    const User new_user(username, password_hash, group, permissions);
    users[username] = new_user;
    
    std::cout << "   Usuario '" << username << "' agregado exitosamente" << std::endl;
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
    } else if (len == 50) {
        handleAuthRequest(peer, data, len, out_response);
    } else if (len == sizeof(GetSystemUsersRequest)) {
        handleGetSystemUsers(peer, data, len);
    } else {
        std::cout << " Longitud no soportada: " << len << " bytes" << std::endl;
    }
}

void AuthUDPServer::handleDiscover(const sockaddr_in& peer, const uint8_t* data, ssize_t len, 
                                  std::string& out_response) {
    const DiscoverRequest* discover = reinterpret_cast<const DiscoverRequest*>(data);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    
    std::cout << " DISCOVER - msgId: " << static_cast<int>(discover->msgId)
              << " desde " << ipStr << std::endl;
    
    DiscoverResponse response;
    response.msgId = discover->msgId;
    response.sensorId = 1;
    response.type = 1;
    response.status = 1;

    out_response.assign(reinterpret_cast<const char*>(&response), sizeof(response));
    
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

            // Verificar si ya existe una sesión con este sessionId
            if (active_sessions.find(sessionId) != active_sessions.end()) {
                // Sesión existente, podemos actualizarla o rechazar
                std::cout << "Sesión existente para sessionId=" << sessionId
                          << ", actualizando..." << std::endl;
            }

            active_sessions[sessionId] = std::make_pair(username, sessionToken);

            std::cout << " Sesión registrada - sessionId: " << sessionId
                      << ", usuario: " << username
                      << ", token: " << sessionToken.substr(0, 8) << "..." << std::endl;
        }
        
        std::cout << " Autenticación EXITOSA: " << username << std::endl;
        std::cout << "   Token de sesión: " << sessionToken << std::endl;
    } else {
        response.setStatusCode(0);
        response.setMessage("Invalid credentials");
        response.setSessionToken("");
        
        // Incrementar intentos fallidos si el usuario existe
        if (it != users.end()) {
            if (it->second.isLocked()) {
                std::cout << " Cuenta BLOQUEADA: " << username << std::endl;
            }
        }
        
        std::cout << " Autenticación FALLIDA: " << username << std::endl;
    }

    // Serializar AuthResponse a buffer
    auto response_buffer = response.toBuffer();
    out_response.assign(response_buffer.begin(), response_buffer.end());
    sendTo(peer, reinterpret_cast<const uint8_t*>(response_buffer.data()), response_buffer.size());

    
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

void AuthUDPServer::handleGetSystemUsers(const sockaddr_in& peer,
                                         const uint8_t* data,
                                         ssize_t len) {
    if (len != sizeof(GetSystemUsersRequest)) {
        std::cout << "GET_SYSTEM_USERS: Tamaño incorrecto" << std::endl;
        return;
    }

    GetSystemUsersRequest req;
    std::memcpy(&req, data, sizeof(req));
    req.toHostOrder();

    uint16_t sessionId = req.sessionId; // sessionId está en host byte order

    // Validar sesión activa - AHORA ES MÁS SIMPLE
    bool sessionValid = false;
    std::string username;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        auto it = active_sessions.find(sessionId);
        if (it != active_sessions.end()) {
            sessionValid = true;
            username = it->second.first; // Extraer el username
            std::cout << "Sesión válida para sessionId=" << sessionId
                      << ", usuario: " << username << std::endl;
        }
    }

    if (!sessionValid) {
        std::cout << "GET_USERS denegado. Sesión inválida: sessionId="
                  << sessionId << std::endl;

        // Opcional: enviar respuesta de error
        GetSystemUsersResponse errorResp{};
        errorResp.msgId = 0x21;
        errorResp.sessionId = sessionId;
        errorResp.totalUsers = 0;
        errorResp.currentIndex = 0;
        errorResp.permissions = 0;

        sendTo(peer, reinterpret_cast<uint8_t*>(&errorResp), sizeof(errorResp));
        return;
    }

    std::cout << "GET_SYSTEM_USERS solicitado por sessionId="
              << sessionId << " (usuario: " << username << ")" << std::endl;

    // std::lock_guard<std::mutex> users_lock(users_mutex);
    // auto userIt = users.find(username);
    // if (userIt == users.end() ||
    //     (userIt->second.getPermissions() & User::PERM_VIEW_USERS) == 0) {
    //     std::cout << "Permisos insuficientes para usuario: " << username << std::endl;
    //     return;
    // }

    // Obtener número total de usuarios
    size_t totalUsers = users.size();
    uint16_t total = static_cast<uint16_t>(totalUsers);

    uint16_t index = 0;

    // Enviar cada usuario en un paquete separado
    for (const auto& kv : users) {
        const User& u = kv.second;

        GetSystemUsersResponse resp{};
        resp.msgId = 0x21;
        resp.sessionId = sessionId; // Incluir sessionId en respuesta
        resp.totalUsers = total;
        resp.currentIndex = index;

        std::memset(resp.username, 0, GetSystemUsersResponse::USERNAME_SIZE);
        std::memset(resp.group, 0, GetSystemUsersResponse::GROUP_SIZE);

        // Usar strncpy para evitar overflow
        std::strncpy(resp.username, u.getUsername().c_str(),
                    GetSystemUsersResponse::USERNAME_SIZE - 1);
        resp.username[GetSystemUsersResponse::USERNAME_SIZE - 1] = '\0';

        std::strncpy(resp.group, u.getGroup().c_str(),
                    GetSystemUsersResponse::GROUP_SIZE - 1);
        resp.group[GetSystemUsersResponse::GROUP_SIZE - 1] = '\0';

        resp.permissions = u.getPermissions();
        resp.toNetworkOrder();

        // Enviar respuesta
        sendTo(peer, reinterpret_cast<uint8_t*>(&resp), sizeof(resp));

        std::cout << "  Enviado usuario #" << index << ": "
                  << u.getUsername() << " (" << u.getGroup() << ")" << std::endl;

        index++;
    }

    std::cout << " Total enviado: " << totalUsers << " paquetes GetSystemUsersResponse."
              << std::endl;
}