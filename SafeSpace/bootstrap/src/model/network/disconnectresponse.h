#ifndef DISCONNECTRESPONSE_H
#define DISCONNECTRESPONSE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

/**
 * @class DisconnectResponse
 * @brief Server response to a DISCONNECT request.
 *
 * Layout:
 *  - MSG_ID (1 byte)       : 0x54 ('T')
 *  - STATUS (1 byte)       : 0=FAIL, 1=SUCCESS
 *  - RESERVED (2 bytes)
 *  - MESSAGE (64 bytes)    : Null-padded string
 */
class DisconnectResponse {
public:
  static constexpr std::uint8_t IDENTIFIER = 0x54;
  static constexpr std::size_t MESSAGE_SIZE = 64;
  static constexpr std::size_t BUFFER_SIZE = 1 + 1 + 2 + MESSAGE_SIZE;
  
  DisconnectResponse() noexcept {
    std::memset(message_.data(), 0, MESSAGE_SIZE);
  }
  
  DisconnectResponse(std::uint8_t status,
                     const std::string &msg) noexcept
      : status_(status) {
    std::memset(message_.data(), 0, MESSAGE_SIZE);
    std::memcpy(message_.data(), msg.data(),
                std::min(msg.size(), MESSAGE_SIZE));
  }
  
  std::uint8_t getStatus() const noexcept { return status_; }
  
  std::string getMessage() const noexcept {
    return std::string(reinterpret_cast<const char*>(message_.data()));
  }
  
  void setStatus(std::uint8_t st) noexcept { status_ = st; }
  
  void setMessage(const std::string &msg) noexcept {
    std::memset(message_.data(), 0, MESSAGE_SIZE);
    std::memcpy(message_.data(), msg.data(),
                std::min(msg.size(), MESSAGE_SIZE));
  }
  
  std::array<std::uint8_t, BUFFER_SIZE> toBuffer() const noexcept {
    std::array<std::uint8_t, BUFFER_SIZE> buf{};
    buf[0] = IDENTIFIER;
    buf[1] = status_;
    buf[2] = 0;
    buf[3] = 0;
    std::memcpy(&buf[4], message_.data(), MESSAGE_SIZE);
    return buf;
  }
  
  bool operator==(const DisconnectResponse &other) const noexcept {
    return status_ == other.status_
           && message_ == other.message_;
  }
  
private:
  std::uint8_t status_ = 0;
  std::array<std::uint8_t, MESSAGE_SIZE> message_{};
};

inline std::ostream& operator<<(std::ostream& os,
                                const DisconnectResponse& resp) {
  os << "[DISCONNECT_RESPONSE status="
     << unsigned(resp.getStatus())
     << " message=\"" << resp.getMessage() << "\"]";
  return os;
}

#endif // DISCONNECTRESPONSE_H
