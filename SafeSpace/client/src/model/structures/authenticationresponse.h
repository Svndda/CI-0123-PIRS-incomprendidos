#ifndef AUTHENTICATIONRESPONSE_H
#define AUTHENTICATIONRESPONSE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <string>

class AuthResponse {
public:
  static constexpr std::size_t MESSAGE_SIZE = 32;
  static constexpr std::size_t TOKEN_SIZE = 16;

  AuthResponse() = default;

  AuthResponse(std::uint16_t sessionId,
               std::uint8_t statusCode,
               const std::string& message,
               const std::string& sessionToken) noexcept
      : sessionId_(sessionId), statusCode_(statusCode) {
    setMessage(message);
    setSessionToken(sessionToken);
  }

  std::uint16_t getSessionId() const noexcept { return sessionId_; }
  std::uint8_t getStatusCode() const noexcept { return statusCode_; }
  std::string getMessage() const noexcept {
    return std::string(message_.data(), strnlen(message_.data(), MESSAGE_SIZE));
  }
  std::string getSessionToken() const noexcept {
    return std::string(sessionToken_.data(), strnlen(sessionToken_.data(), TOKEN_SIZE));
  }

  void setSessionId(std::uint16_t id) noexcept { sessionId_ = id; }
  void setStatusCode(std::uint8_t code) noexcept { statusCode_ = code; }

  void setMessage(const std::string& msg) noexcept {
    std::memset(message_.data(), 0, MESSAGE_SIZE);
    std::memcpy(message_.data(), msg.c_str(), std::min(msg.size(), MESSAGE_SIZE));
  }

  void setSessionToken(const std::string& token) noexcept {
    std::memset(sessionToken_.data(), 0, TOKEN_SIZE);
    std::memcpy(sessionToken_.data(), token.c_str(), std::min(token.size(), TOKEN_SIZE));
  }

  /// Serializa a buffer binario de 51 bytes
  std::array<std::uint8_t, 51> toBuffer() const noexcept {
    std::array<std::uint8_t, 51> buffer{};
    buffer[0] = sessionId_ >> 8;
    buffer[1] = sessionId_ & 0xFF;
    buffer[2] = statusCode_;
    std::memcpy(buffer.data() + 3, message_.data(), MESSAGE_SIZE);
    std::memcpy(buffer.data() + 3 + MESSAGE_SIZE, sessionToken_.data(), TOKEN_SIZE);
    return buffer;
  }

private:
  std::uint16_t sessionId_ = 0;
  std::uint8_t statusCode_ = 0;
  std::array<char, MESSAGE_SIZE> message_{};
  std::array<char, TOKEN_SIZE> sessionToken_{};
};

inline std::ostream& operator<<(std::ostream& os, const AuthResponse& resp) {
  os << "AuthResponse(sessionId=" << resp.getSessionId()
     << ", statusCode=" << static_cast<int>(resp.getStatusCode())
     << ", message=\"" << resp.getMessage() << "\""
     << ", sessionToken=\"" << resp.getSessionToken() << "\")";
  return os;
}

#endif // AUTHENTICATIONRESPONSE_H