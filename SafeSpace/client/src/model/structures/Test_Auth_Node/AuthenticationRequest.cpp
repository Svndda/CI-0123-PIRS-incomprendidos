#include "AuthenticationRequest.h"
#include <iomanip>

// Constructor
AuthenticationRequest::AuthenticationRequest(uint16_t id, 
                                           const std::string& user, 
                                           const std::string& hash) 
    : msgId(id), username(user), passwordHash(hash) {
    
    // Asegurar que las longitudes no excedan los límites del protocolo
    if (username.length() > MAX_USERNAME_LEN) {
        username.resize(MAX_USERNAME_LEN);
    }
    if (passwordHash.length() > MAX_HASH_LEN) {
        passwordHash.resize(MAX_HASH_LEN);
    }
}

// Getters
uint16_t AuthenticationRequest::getMsgId() const { 
    return msgId; 
}

const std::string& AuthenticationRequest::getUsername() const { 
    return username; 
}

const std::string& AuthenticationRequest::getPasswordHash() const { 
    return passwordHash; 
}

// Sobrecarga de operador de inserción
std::ostream& operator<<(std::ostream& os, const AuthenticationRequest& req) {
    os << "AuthenticationRequest { "
       << "msgId: " << req.msgId << ", "
       << "username: '" << req.username << "', "
       << "hash: '" << req.passwordHash.substr(0, 8) << "...' }";
    return os;
}