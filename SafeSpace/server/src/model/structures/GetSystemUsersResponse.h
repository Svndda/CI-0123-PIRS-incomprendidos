#pragma once

#pragma pack(push, 1)
#include <cstdint>

struct GetSystemUsersResponse {
  uint8_t   msgId = 0x21;
  uint16_t sessionId;   // ID de sesión del cliente autenticado (big endian)
  uint16_t  totalUsers;
  uint16_t  currentIndex;

  static constexpr size_t USERNAME_SIZE = 16;
  static constexpr size_t GROUP_SIZE    = 16;

  char username[USERNAME_SIZE];
  char group[GROUP_SIZE];

  uint8_t permissions;

  void toNetworkOrder() {
    sessionId = htons(sessionId);
    totalUsers = htons(totalUsers);
    currentIndex = htons(currentIndex);
  }

  void toHostOrder() {
    sessionId = ntohs(sessionId);
    totalUsers = ntohs(totalUsers);
    currentIndex = ntohs(currentIndex);
  }
};
#pragma pack(pop)
