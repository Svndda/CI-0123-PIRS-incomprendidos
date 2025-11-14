#ifndef CONNECTRESPONSE_H
#define CONNECTRESPONSE_H

#include <array>
#include <cstdint>
#include <ostream>

/**
 * @class ConnectResponse
 * @brief Represents the server's response to a CONNECT request.
 *
 * Layout:
 *  - MSG_ID (1 byte)    : 0x53  (ASCII 'S' for "Server Response")
 *  - STATUS (1 byte)    : 0 = FAIL, 1 = SUCCESS
 *  - SESSION_ID (2 bytes)
 *  - SENSOR_ID  (2 bytes)
 */
class ConnectResponse {
public:
  static constexpr std::uint8_t IDENTIFIER = 0x53;
  static constexpr std::size_t BUFFER_SIZE = 6;
  
  ConnectResponse() = default;
  
  ConnectResponse(std::uint8_t status,
                  std::uint16_t sessionId,
                  std::uint16_t sensorId) noexcept
      : status_(status), sessionId_(sessionId), sensorId_(sensorId) {}
  
  std::uint8_t getStatus() const noexcept { return status_; }
  std::uint16_t getSessionId() const noexcept { return sessionId_; }
  std::uint16_t getSensorId() const noexcept { return sensorId_; }
  
  void setStatus(std::uint8_t status) noexcept { status_ = status; }
  void setSessionId(std::uint16_t id) noexcept { sessionId_ = id; }
  void setSensorId(std::uint16_t id) noexcept { sensorId_ = id; }
  
  std::array<std::uint8_t, BUFFER_SIZE> toBuffer() const noexcept {
    std::array<std::uint8_t, BUFFER_SIZE> buf{};
    buf[0] = IDENTIFIER;
    buf[1] = status_;
    buf[2] = static_cast<std::uint8_t>(sessionId_ >> 8);
    buf[3] = static_cast<std::uint8_t>(sessionId_ & 0xFF);
    buf[4] = static_cast<std::uint8_t>(sensorId_ >> 8);
    buf[5] = static_cast<std::uint8_t>(sensorId_ & 0xFF);
    return buf;
  }
  
  bool operator==(const ConnectResponse &other) const noexcept {
    return status_ == other.status_
           && sessionId_ == other.sessionId_
           && sensorId_ == other.sensorId_;
  }
  
private:
  std::uint8_t status_ = 0;
  std::uint16_t sessionId_ = 0;
  std::uint16_t sensorId_ = 0;
};

inline std::ostream& operator<<(std::ostream &os,
                                const ConnectResponse &resp) {
  os << "[CONNECT_RESPONSE status=" << unsigned(resp.getStatus())
  << " sessionId=" << resp.getSessionId()
  << " sensorId=" << resp.getSensorId()
  << "]";
  return os;
}

#endif // CONNECTRESPONSE_H
