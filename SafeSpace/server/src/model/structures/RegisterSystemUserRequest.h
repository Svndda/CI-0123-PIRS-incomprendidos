#ifndef REGISTERSYSTEMUSERREQUEST_H
#define REGISTERSYSTEMUSERREQUEST_H

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

/**
 * @class RegisterSystemUserRequest
 * @brief Datagram to register a new system user.
 * 
 * Binary format (fixed 90 bytes):
 * - sessionToken: 16 bytes (token from authentication)
 * - username: 16 bytes
 * - password: 32 bytes
 * - group: 16 bytes
 * - permissions: 2 bytes (uint16_t)
 * - padding: 8 bytes
 * Total: 90 bytes
 */
class RegisterSystemUserRequest {
public:
  static constexpr std::size_t SESSION_TOKEN_SIZE = 16;
  static constexpr std::size_t USERNAME_SIZE = 16;
  static constexpr std::size_t PASSWORD_SIZE = 32;
  static constexpr std::size_t GROUP_SIZE = 16;
  static constexpr std::size_t TOTAL_SIZE = 90;

  RegisterSystemUserRequest() = default;

  RegisterSystemUserRequest(const std::string& token,
                           const std::string& username,
                           const std::string& password,
                           const std::string& group,
                           uint16_t permissions) noexcept {
    setSessionToken(token);
    setUsername(username);
    setPassword(password);
    setGroup(group);
    setPermissions(permissions);
  }

  std::string getSessionToken() const noexcept {
    return std::string(sessionToken_.data(),
                       strnlen(sessionToken_.data(), SESSION_TOKEN_SIZE));
  }

  std::string getUsername() const noexcept {
    return std::string(username_.data(),
                       strnlen(username_.data(), USERNAME_SIZE));
  }

  std::string getPassword() const noexcept {
    return std::string(password_.data(),
                       strnlen(password_.data(), PASSWORD_SIZE));
  }

  std::string getGroup() const noexcept {
    return std::string(group_.data(),
                       strnlen(group_.data(), GROUP_SIZE));
  }

  uint16_t getPermissions() const noexcept { return permissions_; }

  void setSessionToken(const std::string& token) noexcept {
    std::memset(sessionToken_.data(), 0, SESSION_TOKEN_SIZE);
    std::memcpy(sessionToken_.data(), token.c_str(),
                std::min(token.size(), SESSION_TOKEN_SIZE));
  }

  void setUsername(const std::string& username) noexcept {
    std::memset(username_.data(), 0, USERNAME_SIZE);
    std::memcpy(username_.data(), username.c_str(),
                std::min(username.size(), USERNAME_SIZE));
  }

  void setPassword(const std::string& password) noexcept {
    std::memset(password_.data(), 0, PASSWORD_SIZE);
    std::memcpy(password_.data(), password.c_str(),
                std::min(password.size(), PASSWORD_SIZE));
  }

  void setGroup(const std::string& group) noexcept {
    std::memset(group_.data(), 0, GROUP_SIZE);
    std::memcpy(group_.data(), group.c_str(),
                std::min(group.size(), GROUP_SIZE));
  }

  void setPermissions(uint16_t perms) noexcept { permissions_ = perms; }

  std::array<uint8_t, TOTAL_SIZE> toBuffer() const noexcept {
    std::array<uint8_t, TOTAL_SIZE> buffer{};
    size_t offset = 0;
    
    std::memcpy(buffer.data() + offset, sessionToken_.data(), SESSION_TOKEN_SIZE);
    offset += SESSION_TOKEN_SIZE;
    
    std::memcpy(buffer.data() + offset, username_.data(), USERNAME_SIZE);
    offset += USERNAME_SIZE;
    
    std::memcpy(buffer.data() + offset, password_.data(), PASSWORD_SIZE);
    offset += PASSWORD_SIZE;
    
    std::memcpy(buffer.data() + offset, group_.data(), GROUP_SIZE);
    offset += GROUP_SIZE;
    
    buffer[offset] = (permissions_ >> 8) & 0xFF;
    buffer[offset + 1] = permissions_ & 0xFF;
    offset += 2;
    
    // Rest is padding (already zeroed)
    return buffer;
  }

  static RegisterSystemUserRequest fromBuffer(const uint8_t* data, size_t len) noexcept {
    RegisterSystemUserRequest req;
    if (len < TOTAL_SIZE) return req;

    size_t offset = 0;
    std::array<char, SESSION_TOKEN_SIZE> token;
    std::memcpy(token.data(), data + offset, SESSION_TOKEN_SIZE);
    req.sessionToken_ = token;
    offset += SESSION_TOKEN_SIZE;

    std::array<char, USERNAME_SIZE> username;
    std::memcpy(username.data(), data + offset, USERNAME_SIZE);
    req.username_ = username;
    offset += USERNAME_SIZE;

    std::array<char, PASSWORD_SIZE> password;
    std::memcpy(password.data(), data + offset, PASSWORD_SIZE);
    req.password_ = password;
    offset += PASSWORD_SIZE;

    std::array<char, GROUP_SIZE> group;
    std::memcpy(group.data(), data + offset, GROUP_SIZE);
    req.group_ = group;
    offset += GROUP_SIZE;

    req.permissions_ = (static_cast<uint16_t>(data[offset]) << 8) |
                       static_cast<uint16_t>(data[offset + 1]);

    return req;
  }

private:
  std::array<char, SESSION_TOKEN_SIZE> sessionToken_{};
  std::array<char, USERNAME_SIZE> username_{};
  std::array<char, PASSWORD_SIZE> password_{};
  std::array<char, GROUP_SIZE> group_{};
  uint16_t permissions_ = 0;
};

#endif // REGISTERSYSTEMUSERREQUEST_H
