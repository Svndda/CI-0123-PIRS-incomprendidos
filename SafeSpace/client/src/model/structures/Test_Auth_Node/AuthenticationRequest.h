#ifndef AUTHENTICATION_REQUEST_H
#define AUTHENTICATION_REQUEST_H

#include <cstdint>
#include <string>
#include <iostream>

// Mensaje de solicitud de autenticación:
// MSG_ID (16 bits), USERNAME (8-16 bytes), Hash de contraseña (8-32 bytes)
class AuthenticationRequest {
private:
    uint16_t msgId;
    std::string username;
    std::string passwordHash;

public:
    static const int MAX_USERNAME_LEN = 16;
    static const int MAX_HASH_LEN = 32;

    AuthenticationRequest(uint16_t id = 0, 
                          const std::string& user = "", 
                          const std::string& hash = "");

    // Getters
    uint16_t getMsgId() const;
    const std::string& getUsername() const;
    const std::string& getPasswordHash() const;
    
    // Sobrecarga de operador de inserción para imprimir
    friend std::ostream& operator<<(std::ostream& os, const AuthenticationRequest& req);
};

#endif // AUTHENTICATION_REQUEST_H