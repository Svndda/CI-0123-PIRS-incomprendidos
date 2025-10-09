/*#include "disconnectrequest.h"

DisconnectRequest::DisconnectRequest(std::uint16_t sessionId,
                                     std::uint16_t sensorId,
                                     std::uint8_t flagBits) noexcept
                                    : sessionId_(sessionId),
                                        sensorId_(sensorId),
                                        flagBits_(flagBits) {}

                    
std::uint16_t DisconnectRequest::getSessionId() const noexcept { 
  return sessionId_; 
}
std::uint16_t DisconnectRequest::getSensorId() const noexcept { 
  return sensorId_; 
}
std::uint8_t DisconnectRequest::getFlagBits() const noexcept { 
  return flagBits_; 
}


std::array<std::uint8_t, 5> DisconnectRequest::toBuffer() const noexcept {
    std::array<std::uint8_t, 5> buf{};
    buf[0] = static_cast<std::uint8_t>(sessionId_ >> 8);
    buf[1] = static_cast<std::uint8_t>(sessionId_ & 0xFF);
    buf[2] = static_cast<std::uint8_t>(sensorId_ >> 8);
    buf[3] = static_cast<std::uint8_t>(sensorId_ & 0xFF);
    buf[4] = flagBits_;
    return buf;
}

 bool DisconnectRequest::operator==(const DisconnectRequest& other) const noexcept {
    return sessionId_ == other.sessionId_ &&
           sensorId_ == other.sensorId_ &&
           flagBits_ == other.flagBits_;
  }

  bool DisconnectRequest::operator!=(const DisconnectRequest& other) const noexcept {
    return !(*this == other);
  }
