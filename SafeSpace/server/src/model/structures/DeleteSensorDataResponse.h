#ifndef DELETE_SENSOR_DATA_RESPONSE_H
#define DELETE_SENSOR_DATA_RESPONSE_H

#include <cstdint>
#include <iostream>
#include <vector>

class DeleteSensorDataResponse {
public:
  static constexpr uint8_t MSG_ID = 0x95;
  uint16_t sessionId{};   // ID de sesión (big endian)
  uint8_t status{};

  /**
   * @brief Parses a DELETE_SENSOR_DATA_RESPONSE from raw bytes.
   */
  static DeleteSensorDataResponse fromBytes(const uint8_t* data, ssize_t len) {
    DeleteSensorDataResponse resp;

    if (len < 4) { // Mínimo: MSG_ID(1) + sessionId(2) + status(1)
      std::cerr << "DELETE_SENSOR_DATA_RESPONSE: Tamaño insuficiente: " << len << std::endl;
      resp.status = 1; // Error
      return resp;
    }

    // Verificar ID del mensaje
    if (data[0] != MSG_ID) {
      std::cerr << "DELETE_SENSOR_DATA_RESPONSE: ID incorrecto: "
                << static_cast<int>(data[0]) << std::endl;
      resp.status = 1;
      return resp;
    }

    // Extraer sessionId (big endian)
    resp.sessionId = (data[1] << 8) | data[2];

    // Extraer status
    resp.status = data[3];

    return resp;
  }

  /**
   * @brief Converts response to bytes for sending
   */
  std::vector<uint8_t> toBytes() const {
    std::vector<uint8_t> bytes(4);
    bytes[0] = MSG_ID;
    bytes[1] = static_cast<uint8_t>((sessionId >> 8) & 0xFF);
    bytes[2] = static_cast<uint8_t>(sessionId & 0xFF);
    bytes[3] = status;
    return bytes;
  }
};

#endif // DELETE_SENSOR_DATA_RESPONSE_H