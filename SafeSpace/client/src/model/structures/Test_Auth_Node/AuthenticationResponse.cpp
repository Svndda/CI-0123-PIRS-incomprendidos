#include "AuthenticationResponse.h"
#include <iomanip>

// Constructor
AuthenticationResponse::AuthenticationResponse(uint16_t id, 
                                               uint16_t type, 
                                               uint16_t dataVal)
    : msgId(id), dataType(type), data(dataVal) {}

// Getters
uint16_t AuthenticationResponse::getMsgId() const { 
    return msgId; 
}

uint16_t AuthenticationResponse::getDataType() const { 
    return dataType; 
}

uint16_t AuthenticationResponse::getData() const { 
    return data; 
}

// Setters
void AuthenticationResponse::setMsgId(uint16_t id) { 
    msgId = id; 
}

void AuthenticationResponse::setDataType(uint16_t type) { 
    dataType = type; 
}

void AuthenticationResponse::setData(uint16_t dataVal) { 
    data = dataVal; 
}

// Sobrecarga de operador de inserción
std::ostream& operator<<(std::ostream& os, const AuthenticationResponse& resp) {
    os << "AuthenticationResponse { "
       << "msgId: " << resp.msgId << ", "
       << "dataType: " << std::hex << "0x" << resp.dataType << std::dec << ", "
       << "data: " << resp.data << " (" 
       << (resp.data == AuthenticationResponse::LOGIN_SUCCESS ? "SUCCESS" : 
           (resp.data == AuthenticationResponse::LOGIN_FAILURE ? "FAILURE" : "Value"))
       << ") }";
    return os;
}