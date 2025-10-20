#ifndef AUTHENTICATIONREQUEST_H
#define AUTHENTICATIONREQUEST_H

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

/**
 * @class AuthRequest
 * @brief Holds the payload of an AUTHENTICATION_REQUEST message.
 */
class AuthRequest {
public:
  static constexpr std::size_t USERNAME_SIZE = 16;
  static constexpr std::size_t PASSWORD_SIZE = 32;

  AuthRequest() = default;

  AuthRequest(std::uint16_t sessionId,
              const std::string& username,
              const std::string& password) noexcept {
    setSessionId(sessionId);
    setUsername(username);
    setPassword(password);
  }

  std::uint16_t getSessionId() const noexcept { return sessionId_; }
  std::string getUsername() const noexcept {
    return std::string(username_.data(),
                       strnlen(username_.data(), USERNAME_SIZE));
  }
  std::string getPassword() const noexcept {
    return std::string(password_.data(),
                       strnlen(password_.data(), PASSWORD_SIZE));
  }

  void setSessionId(std::uint16_t sessionId) noexcept { sessionId_ = sessionId; }

  void setUsername(const std::string& username) noexcept {
    std::memset(username_.data(), 0, USERNAME_SIZE);
    std::memcpy(username_.data(),
                username.c_str(),
                std::min(username.size(), USERNAME_SIZE));
  }

  void setPassword(const std::string& password) noexcept {
    std::memset(password_.data(), 0, PASSWORD_SIZE);
    std::memcpy(password_.data(),
                password.c_str(),
                std::min(password.size(), PASSWORD_SIZE));
  }

  /// Serializa a buffer binario fijo de 50 bytes
  std::array<std::uint8_t, 50> toBuffer() const noexcept {
    std::array<std::uint8_t, 50> buffer{};
    buffer[0] = sessionId_ >> 8;
    buffer[1] = sessionId_ & 0xFF;
    std::memcpy(buffer.data() + 2, username_.data(), USERNAME_SIZE);
    std::memcpy(buffer.data() + 2 + USERNAME_SIZE, password_.data(), PASSWORD_SIZE);
    return buffer;
  }

  bool operator==(const AuthRequest& other) const noexcept {
    return sessionId_ == other.sessionId_ &&
           username_ == other.username_ &&
           password_ == other.password_;
  }

  bool operator!=(const AuthRequest& other) const noexcept {
    return !(*this == other);
  }

private:
  std::uint16_t sessionId_ = 0;
  std::array<char, USERNAME_SIZE> username_{};
  std::array<char, PASSWORD_SIZE> password_{};
};

inline std::ostream& operator<<(std::ostream& os, const AuthRequest& req) {
  os << "AuthRequest(sessionId=" << req.getSessionId()
     << ", username=" << req.getUsername()
     << ", password=" << std::string(req.getPassword().size(), '*') << ")";
  return os;
}

#endif // AUTHENTICATIONREQUEST_H