#ifndef AUTHENTICATION_RESPONSE_H
#define AUTHENTICATION_RESPONSE_H

#include <cstdint>
#include <iostream>

// Mensaje de respuesta de autenticación:
// MSG_ID (16 bits), DATA_TYPE (16 bits), DATA (16 bits)
class AuthenticationResponse {
private:
    uint16_t msgId;
    uint16_t dataType;
    uint16_t data;

public:
    // Definiciones de tipos de datos para DATA_TYPE
    enum DataType : uint16_t {
        AUTH_SUCCESS = 0x0001,
        AUTH_FAILED = 0x0002,
        SESSION_ID = 0x0003,
        PERMISSIONS = 0x0004
    };
    
    // Definiciones de DATA para casos simples
    enum DataValue : uint16_t {
        LOGIN_SUCCESS = 0x0001,
        LOGIN_FAILURE = 0x0000
    };

    AuthenticationResponse(uint16_t id = 0, 
                           uint16_t type = AUTH_FAILED, 
                           uint16_t dataVal = LOGIN_FAILURE);

    // Getters
    uint16_t getMsgId() const;
    uint16_t getDataType() const;
    uint16_t getData() const;

    // Setters
    void setMsgId(uint16_t id);
    void setDataType(uint16_t type);
    void setData(uint16_t dataVal);

    // Sobrecarga de operador de inserción para imprimir
    friend std::ostream& operator<<(std::ostream& os, const AuthenticationResponse& resp);
};

#endif // AUTHENTICATION_RESPONSE_H