#ifndef GET_SENSOR_DATA_REQUEST_H
#define GET_SENSOR_DATA_REQUEST_H

#include <vector>
#include <cstdint>
#include "Token.h"

/**
 * @brief Represents a GET_SENSOR_DATA_REQUEST datagram.
 */
class GetSensorDataRequest {
public:
  static constexpr uint8_t MSG_ID = 0x90;
  uint16_t sessionId;
  uint16_t sensorId{};
  Token16 token;

  /**
   * @brief Serializes request into a datagram byte buffer.
   */
  std::vector<uint8_t> toBytes() const {
    std::vector<uint8_t> out;
    out.reserve(1 + 2 + 2 + 16); // MSG_ID + sessionId + sensorId + token

    out.push_back(MSG_ID);

    // Serializar sessionId (big endian)
    out.push_back(static_cast<uint8_t>(sessionId >> 8));
    out.push_back(static_cast<uint8_t>(sessionId & 0xFF));

    // Serializar sensorId
    out.push_back(static_cast<uint8_t>(sensorId >> 8));
    out.push_back(static_cast<uint8_t>(sensorId & 0xFF));

    // Serializar token
    token.writeTo(out);

    return out;
  }
};

#endif