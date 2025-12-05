#ifndef DISCONNECTREQUEST_H
#define DISCONNECTREQUEST_H

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

/**
 * @class DisconnectRequest
 * @brief Client request to log out / end the session.
 *
 * Layout:
 *  - MSG_ID (1 byte)     : 0x44 ('D')
 *  - SESSION_ID (2 bytes): big-endian
 */
class DisconnectRequest {
 public:
  static constexpr std::uint8_t IDENTIFIER = 0x44;
  static constexpr std::size_t BUFFER_SIZE = 3;

  DisconnectRequest() = default;
  explicit DisconnectRequest(std::uint16_t sessionId) noexcept
      : sessionId_(sessionId) {}

  std::uint16_t getSessionId() const noexcept { return sessionId_; }
  void setSessionId(std::uint16_t sid) noexcept { sessionId_ = sid; }

  std::array<std::uint8_t, BUFFER_SIZE> toBuffer() const noexcept {
    std::array<std::uint8_t, BUFFER_SIZE> buf{};
    buf[0] = IDENTIFIER;
    buf[1] = static_cast<std::uint8_t>(sessionId_ >> 8);
    buf[2] = static_cast<std::uint8_t>(sessionId_ & 0xFF);
    return buf;
  }

  bool operator==(const DisconnectRequest& other) const noexcept {
    return sessionId_ == other.sessionId_;
  }

 private:
  std::uint16_t sessionId_ = 0;
};

inline std::ostream& operator<<(std::ostream& os,
                                const DisconnectRequest& req) {
  os << "[DISCONNECT_REQUEST sessionId=" << req.getSessionId() << "]";
  return os;
}

#endif  // DISCONNECTREQUEST_H
