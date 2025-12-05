// GetSystemUsersResponse.h
#ifndef GETSYSTEMUSERSRESPONSE_H
#define GETSYSTEMUSERSRESPONSE_H

#include <cstdint>
#include <string>
#include <cstring>
#include <stdexcept>
#include <QByteArray>

/**
 * @brief Network wire-format for GetSystemUsersResponse (fixed 40 bytes)
 *
 * Layout (indices):
 * [0]       uint8_t  msgId
 * [1..2]    uint16_t sessionId  (big-endian)
 * [3..4]    uint16_t totalUsers (big-endian)
 * [5..6]    uint16_t currentIndex (big-endian)
 * [7..22]   char[16] username (no guaranteed '\0', padded)
 * [23..38]  char[16] group    (no guaranteed '\0', padded)
 * [39]      uint8_t  permissions
 */
struct GetSystemUsersResponse {
  uint8_t  msgId;
  uint16_t sessionId;
  uint16_t totalUsers;
  uint16_t currentIndex;
  
  static constexpr size_t USERNAME_SIZE = 16;
  static constexpr size_t GROUP_SIZE = 16;
  
  char username[USERNAME_SIZE];
  char group[GROUP_SIZE];
  uint8_t permissions;
  
  static constexpr size_t SIZE = 1 + 2 + 2 + 2 + USERNAME_SIZE + GROUP_SIZE + 1;
  
  GetSystemUsersResponse()
      : msgId(0), sessionId(0), totalUsers(0), currentIndex(0), permissions(0) {
    std::memset(username, 0, USERNAME_SIZE);
    std::memset(group, 0, GROUP_SIZE);
  }
  
  /**
   * @brief Parse from a raw buffer (does NOT rely on struct packing)
   * @param buffer pointer to at least SIZE bytes
   * @throws std::runtime_error on invalid data
   */
  static GetSystemUsersResponse parseFromBuffer(const uint8_t* buffer, size_t len) {
    if (!buffer) throw std::runtime_error("null buffer");
    if (len < SIZE) throw std::runtime_error("buffer too small for GetSystemUsersResponse");
    
    GetSystemUsersResponse r;
    
    // msgId
    r.msgId = buffer[0];
    
    // big-endian numeric fields (network order -> host order)
    r.sessionId = static_cast<uint16_t>((buffer[1] << 8) | buffer[2]);
    r.totalUsers = static_cast<uint16_t>((buffer[3] << 8) | buffer[4]);
    r.currentIndex = static_cast<uint16_t>((buffer[5] << 8) | buffer[6]);
    
    // copy username and group exactly as on wire (no assumption of '\0')
    std::memcpy(r.username, buffer + 7, USERNAME_SIZE);
    std::memcpy(r.group, buffer + 23, GROUP_SIZE);
    
    // permissions
    r.permissions = buffer[39];
    
    return r;
  }
  
  static GetSystemUsersResponse parseFromQByteArray(const QByteArray& a) {
    if (static_cast<size_t>(a.size()) < SIZE) throw std::runtime_error("QByteArray too small");
    return parseFromBuffer(reinterpret_cast<const uint8_t*>(a.constData()), a.size());
  }
  
  // helpers to get strings trimmed at first '\0' or trimmed whitespace
  std::string getTrimmedUsername() const {
    std::string s(username, USERNAME_SIZE);
    size_t pos = s.find('\0');
    if (pos != std::string::npos) s.resize(pos);
    // trim trailing spaces
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
  }
  
  std::string getTrimmedGroup() const {
    std::string s(group, GROUP_SIZE);
    size_t pos = s.find('\0');
    if (pos != std::string::npos) s.resize(pos);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
  }
};

#endif // GETSYSTEMUSERSRESPONSE_H
