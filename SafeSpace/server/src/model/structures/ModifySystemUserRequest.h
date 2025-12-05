#ifndef MODIFYSYSTEMUSERREQUEST_H
#define MODIFYSYSTEMUSERREQUEST_H

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

/**
 * @class ModifySystemUserRequest
 * @brief Datagram to modify an existing system user.
 * 
 * Binary format (fixed 90 bytes):
 * - sessionToken: 16 bytes (token from authentication; requester must be admin)
 * - targetUsername: 16 bytes (username to modify)
 * - newPassword: 32 bytes (if empty, password unchanged)
 * - newGroup: 16 bytes (if empty, group unchanged)
 * - newPermissions: 2 bytes (uint16_t; if 0, unchanged)
 * - padding: 8 bytes
 * Total: 90 bytes
 */
class ModifySystemUserRequest {
public:
  static constexpr std::size_t SESSION_TOKEN_SIZE = 16;
  static constexpr std::size_t TARGET_USERNAME_SIZE = 16;
  static constexpr std::size_t NEW_PASSWORD_SIZE = 32;
  static constexpr std::size_t NEW_GROUP_SIZE = 16;
  static constexpr std::size_t TOTAL_SIZE = 90;

  ModifySystemUserRequest() = default;

  ModifySystemUserRequest(const std::string& token,
                         const std::string& targetUsername,
                         const std::string& newPassword,
                         const std::string& newGroup,
                         uint16_t newPermissions) noexcept {
    setSessionToken(token);
    setTargetUsername(targetUsername);
    setNewPassword(newPassword);
    setNewGroup(newGroup);
    setNewPermissions(newPermissions);
  }

  std::string getSessionToken() const noexcept {
    return std::string(sessionToken_.data(),
                       strnlen(sessionToken_.data(), SESSION_TOKEN_SIZE));
  }

  std::string getTargetUsername() const noexcept {
    return std::string(targetUsername_.data(),
                       strnlen(targetUsername_.data(), TARGET_USERNAME_SIZE));
  }

  std::string getNewPassword() const noexcept {
    return std::string(newPassword_.data(),
                       strnlen(newPassword_.data(), NEW_PASSWORD_SIZE));
  }

  std::string getNewGroup() const noexcept {
    return std::string(newGroup_.data(),
                       strnlen(newGroup_.data(), NEW_GROUP_SIZE));
  }

  uint16_t getNewPermissions() const noexcept { return newPermissions_; }

  void setSessionToken(const std::string& token) noexcept {
    std::memset(sessionToken_.data(), 0, SESSION_TOKEN_SIZE);
    std::memcpy(sessionToken_.data(), token.c_str(),
                std::min(token.size(), SESSION_TOKEN_SIZE));
  }

  void setTargetUsername(const std::string& username) noexcept {
    std::memset(targetUsername_.data(), 0, TARGET_USERNAME_SIZE);
    std::memcpy(targetUsername_.data(), username.c_str(),
                std::min(username.size(), TARGET_USERNAME_SIZE));
  }

  void setNewPassword(const std::string& password) noexcept {
    std::memset(newPassword_.data(), 0, NEW_PASSWORD_SIZE);
    std::memcpy(newPassword_.data(), password.c_str(),
                std::min(password.size(), NEW_PASSWORD_SIZE));
  }

  void setNewGroup(const std::string& group) noexcept {
    std::memset(newGroup_.data(), 0, NEW_GROUP_SIZE);
    std::memcpy(newGroup_.data(), group.c_str(),
                std::min(group.size(), NEW_GROUP_SIZE));
  }

  void setNewPermissions(uint16_t perms) noexcept { newPermissions_ = perms; }

  std::array<uint8_t, TOTAL_SIZE> toBuffer() const noexcept {
    std::array<uint8_t, TOTAL_SIZE> buffer{};
    size_t offset = 0;
    
    std::memcpy(buffer.data() + offset, sessionToken_.data(), SESSION_TOKEN_SIZE);
    offset += SESSION_TOKEN_SIZE;
    
    std::memcpy(buffer.data() + offset, targetUsername_.data(), TARGET_USERNAME_SIZE);
    offset += TARGET_USERNAME_SIZE;
    
    std::memcpy(buffer.data() + offset, newPassword_.data(), NEW_PASSWORD_SIZE);
    offset += NEW_PASSWORD_SIZE;
    
    std::memcpy(buffer.data() + offset, newGroup_.data(), NEW_GROUP_SIZE);
    offset += NEW_GROUP_SIZE;
    
    buffer[offset] = (newPermissions_ >> 8) & 0xFF;
    buffer[offset + 1] = newPermissions_ & 0xFF;
    offset += 2;
    
    // Rest is padding (already zeroed)
    return buffer;
  }

  static ModifySystemUserRequest fromBuffer(const uint8_t* data, size_t len) noexcept {
    ModifySystemUserRequest req;
    if (len < TOTAL_SIZE) return req;

    size_t offset = 0;
    std::array<char, SESSION_TOKEN_SIZE> token;
    std::memcpy(token.data(), data + offset, SESSION_TOKEN_SIZE);
    req.sessionToken_ = token;
    offset += SESSION_TOKEN_SIZE;

    std::array<char, TARGET_USERNAME_SIZE> targetUsername;
    std::memcpy(targetUsername.data(), data + offset, TARGET_USERNAME_SIZE);
    req.targetUsername_ = targetUsername;
    offset += TARGET_USERNAME_SIZE;

    std::array<char, NEW_PASSWORD_SIZE> newPassword;
    std::memcpy(newPassword.data(), data + offset, NEW_PASSWORD_SIZE);
    req.newPassword_ = newPassword;
    offset += NEW_PASSWORD_SIZE;

    std::array<char, NEW_GROUP_SIZE> newGroup;
    std::memcpy(newGroup.data(), data + offset, NEW_GROUP_SIZE);
    req.newGroup_ = newGroup;
    offset += NEW_GROUP_SIZE;

    req.newPermissions_ = (static_cast<uint16_t>(data[offset]) << 8) |
                          static_cast<uint16_t>(data[offset + 1]);

    return req;
  }

private:
  std::array<char, SESSION_TOKEN_SIZE> sessionToken_{};
  std::array<char, TARGET_USERNAME_SIZE> targetUsername_{};
  std::array<char, NEW_PASSWORD_SIZE> newPassword_{};
  std::array<char, NEW_GROUP_SIZE> newGroup_{};
  uint16_t newPermissions_ = 0;
};

#endif // MODIFYSYSTEMUSERREQUEST_H
