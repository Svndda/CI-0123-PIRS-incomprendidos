#ifndef DELETE_SENSOR_DATA_RESPONSE_H
#define DELETE_SENSOR_DATA_RESPONSE_H

#include <cstdint>
#include <unistd.h>

/**
 * @brief Represents a DELETE_SENSOR_DATA_RESPONSE datagram.
 */
class DeleteSensorDataResponse {
public:
  static constexpr uint8_t MSG_ID = 0x95;
  uint16_t sessionId;   // ID de sesión del cliente autenticado (big endian)
  uint8_t status{};

  static DeleteSensorDataResponse fromBytes(const uint8_t* data, ssize_t len) {
    DeleteSensorDataResponse r;

    if (len < 4) { // Mínimo: MSG_ID(1) + sessionId(2) + status(1)
      std::cerr << "DELETE_SENSOR_DATA_RESPONSE: Tamaño insuficiente: " << len << std::endl;
      return r;
    }

    // Verificar ID del mensaje
    if (data[0] != MSG_ID) {
      std::cerr << "DELETE_SENSOR_DATA_RESPONSE: ID incorrecto: "
                << static_cast<int>(data[0]) << std::endl;
      return r;
    }

    // Extraer sessionId (big endian)
    r.sessionId = (data[1] << 8) | data[2];

    // Extraer status
    r.status = data[3];

    return r;
  }
};

#endif