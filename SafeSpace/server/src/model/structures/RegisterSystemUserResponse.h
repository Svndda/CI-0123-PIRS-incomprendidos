#ifndef REGISTERSYSTEMUSERRESPONSE_H
#define REGISTERSYSTEMUSERRESPONSE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

/**
 * @class RegisterSystemUserResponse
 * @brief Response to RegisterSystemUserRequest.
 * 
 * Binary format (fixed 35 bytes):
 * - statusCode: 1 byte (0=fail, 1=success)
 * - message: 32 bytes
 * - padding: 2 bytes
 * Total: 35 bytes
 */
class RegisterSystemUserResponse {
public:
  static constexpr std::size_t MESSAGE_SIZE = 32;
  static constexpr std::size_t TOTAL_SIZE = 35;

  RegisterSystemUserResponse() = default;

  RegisterSystemUserResponse(uint8_t statusCode, const std::string& message) noexcept {
    setStatusCode(statusCode);
    setMessage(message);
  }

  uint8_t getStatusCode() const noexcept { return statusCode_; }

  std::string getMessage() const noexcept {
    return std::string(message_.data(), strnlen(message_.data(), MESSAGE_SIZE));
  }

  void setStatusCode(uint8_t code) noexcept { statusCode_ = code; }

  void setMessage(const std::string& msg) noexcept {
    std::memset(message_.data(), 0, MESSAGE_SIZE);
    std::memcpy(message_.data(), msg.c_str(),
                std::min(msg.size(), MESSAGE_SIZE));
  }

  std::array<uint8_t, TOTAL_SIZE> toBuffer() const noexcept {
    std::array<uint8_t, TOTAL_SIZE> buffer{};
    buffer[0] = statusCode_;
    std::memcpy(buffer.data() + 1, message_.data(), MESSAGE_SIZE);
    // Rest is padding (already zeroed)
    return buffer;
  }

  static RegisterSystemUserResponse fromBuffer(const uint8_t* data, size_t len) noexcept {
    RegisterSystemUserResponse resp;
    if (len < TOTAL_SIZE) return resp;

    resp.statusCode_ = data[0];
    std::array<char, MESSAGE_SIZE> msg;
    std::memcpy(msg.data(), data + 1, MESSAGE_SIZE);
    resp.message_ = msg;

    return resp;
  }

private:
  uint8_t statusCode_ = 0;
  std::array<char, MESSAGE_SIZE> message_{};
};

#endif // REGISTERSYSTEMUSERRESPONSE_H
