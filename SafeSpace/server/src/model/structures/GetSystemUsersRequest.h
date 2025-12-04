#pragma once

#pragma pack(push, 1)
#include <cstdint>

struct GetSystemUsersRequest {
  uint8_t  msgId = 0x20;
  uint16_t sessionId;   // ID de sesión del cliente autenticado (big endian)
  uint8_t  reserved;    // Para mantener alineación y extensiones futuras

  void toNetworkOrder() {
    sessionId = htons(sessionId);
  }

  void toHostOrder() {
    sessionId = ntohs(sessionId);
  }
};
#pragma pack(pop)
