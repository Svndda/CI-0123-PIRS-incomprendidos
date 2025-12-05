// GetSystemUsersRequest.h - Versión simplificada
#ifndef GETSYSTEMUSERSREQUEST_H
#define GETSYSTEMUSERSREQUEST_H

#include <cstdint>
#include <cstring>
#include <QByteArray>

struct GetSystemUsersRequest {
  uint8_t msgId = 0x20;   // Byte 0
  uint16_t sessionId;     // Bytes 1-2 (big endian en la red)
  uint8_t reserved;       // Byte 3
  
  static constexpr size_t SIZE = 1 + 2 + 1;
  
  /**
     * @brief Crea un buffer listo para enviar
     */
  static void writeToBuffer(uint8_t* buffer, uint16_t sessionIdValue) {
    buffer[0] = 0x20;  // msgId
    
    // Escribir sessionId directamente en big endian
    buffer[1] = static_cast<uint8_t>((sessionIdValue >> 8) & 0xFF);
    buffer[2] = static_cast<uint8_t>(sessionIdValue & 0xFF);
    
    buffer[3] = 0;  // reserved
  }
  
  /**
     * @brief Crea QByteArray listo para enviar
     */
  static QByteArray createForSending(uint16_t sessionIdValue) {
    QByteArray data(SIZE, 0);
    writeToBuffer(reinterpret_cast<uint8_t*>(data.data()), sessionIdValue);
    return data;
  }
};

#endif // GETSYSTEMUSERSREQUEST_H
