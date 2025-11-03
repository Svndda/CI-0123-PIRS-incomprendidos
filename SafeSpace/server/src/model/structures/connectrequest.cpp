#include "connectrequest.h"


ConnectRequest::ConnectRequest() {}

ConnectRequest::ConnectRequest(std::uint16_t sessionId,
                 std::uint16_t sensorId,
                 std::uint8_t flagBits) noexcept {
  this->sessionId_ = sessionId;
  this->sensorId_ = sensorId;
  this->flagBits_ = flagBits;
}

std::uint16_t ConnectRequest::getSessionId() const noexcept {
  return this->sessionId_;
}

std::uint16_t ConnectRequest::getSensorId() const noexcept {
  return this->sensorId_;
}

std::uint8_t ConnectRequest::getFlagBits() const noexcept {
  return this->flagBits_;
}

void ConnectRequest::setSessionId(std::uint16_t sessionId) noexcept {
  this->sessionId_ = sessionId;
}

void ConnectRequest::setSensorId(std::uint16_t sensorId) noexcept {
  this->sensorId_ = sensorId;
}

void ConnectRequest::setFlagBits(std::uint8_t flagBits) noexcept {
  this->flagBits_ = flagBits;
}

// // Método para verificar si una bandera específica está activa
// bool ConnectRequest::hasFlag(ConnectFlag flag) const noexcept {
//   return (this->flagBits_ & static_cast<std::uint8_t>(flag)) != 0;
// }

// Método para convertir a buffer de bytes
std::array<std::uint8_t, 6> ConnectRequest::toBuffer() const noexcept {
  std::array<std::uint8_t, 6> buffer;
  
  buffer[0] = static_cast<std::uint8_t>(IDENTIFIER & 0xFF);
  
  // Session ID (2 bytes, big-endian / network order)
  buffer[1] = static_cast<std::uint8_t>((this->sessionId_ >> 8) & 0xFF);
  buffer[2] = static_cast<std::uint8_t>(this->sessionId_ & 0xFF);
  
  // Sensor ID (2 bytes, big-endian / network order)
  buffer[3] = static_cast<std::uint8_t>((this->sensorId_ >> 8) & 0xFF);
  buffer[4] = static_cast<std::uint8_t>(this->sensorId_ & 0xFF);
  
  // flagBits (1 byte)
  buffer[5] = this->flagBits_;

  return buffer;
}

bool ConnectRequest::operator==(const ConnectRequest& other) const noexcept {
  return (this->sessionId_ == other.sessionId_)
      && (this->sensorId_ == other.sensorId_)
      && (this->flagBits_ == other.flagBits_);
}

bool ConnectRequest::operator!=(const ConnectRequest& other) const noexcept {
  return (this->sessionId_ != other.sessionId_)
      || (this->sensorId_ != other.sensorId_)
      || (this->flagBits_ != other.flagBits_);
}

std::ostream& operator<<(std::ostream& os, const ConnectRequest& request){
  os << "ConnectRequest { "
     << "sessionId: " << request.getSessionId() << ", "
     << "sensorId: " << request.getSensorId() << ", "
     << "flagBits: " << static_cast<int>(request.getFlagBits())
     << " }";
  return os;
}
