#ifndef CONNECTREQUEST_H
#define CONNECTREQUEST_H

#include <array>
#include <cstdint>
#include <ostream>



/**
 * @class ConnectRequest
 * @brief Holds the payload of a CONNECT message.
 */
class ConnectRequest {
public:
  static constexpr int IDENTIFIER = 0x43;
  ConnectRequest();
  ConnectRequest(std::uint16_t sessionId,
                 std::uint16_t sensorId,
                 std::uint8_t flagBits) noexcept;

  std::uint16_t getSessionId() const noexcept;
  std::uint16_t getSensorId() const noexcept;
  std::uint8_t getFlagBits() const noexcept;


  void setSessionId(std::uint16_t sessionId) noexcept;
  void setSensorId(std::uint16_t sensorId) noexcept;
  void setFlagBits(std::uint8_t flagBits) noexcept;

  std::array<std::uint8_t, 5> toBuffer() const noexcept;

  bool operator==(const ConnectRequest& other) const noexcept;
  bool operator!=(const ConnectRequest& other) const noexcept;

private:
  std::uint16_t sessionId_ = 0;
  std::uint16_t sensorId_ = 0;
  std::uint8_t flagBits_ = 0;
};

std::ostream& operator<<(std::ostream& os, const ConnectRequest& request);

#endif // CONNECTREQUEST_H
