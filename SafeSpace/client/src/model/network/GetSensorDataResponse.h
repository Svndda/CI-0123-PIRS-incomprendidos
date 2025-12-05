#ifndef GET_SENSOR_DATA_RESPONSE_H
#define GET_SENSOR_DATA_RESPONSE_H

#include <vector>
#include <cstdint>
#include <iostream>

/**
 * @brief Represents a GET_SENSOR_DATA_RESPONSE datagram.
 */
class GetSensorDataResponse {
public:
  static constexpr uint8_t MSG_ID = 0x91;
  uint16_t sessionId;   // ID de sesión del cliente autenticado (big endian)
  uint8_t status{};
  std::vector<uint8_t> payload;

  /**
   * @brief Parses a GET_SENSOR_DATA_RESPONSE from raw bytes.
   */
  static GetSensorDataResponse fromBytes(const uint8_t* data, ssize_t len) {
    GetSensorDataResponse resp;

    if (len < 6) { // Mínimo: MSG_ID(1) + sessionId(2) + status(1) + tamaño(2)
      std::cerr << "GET_SENSOR_DATA_RESPONSE: Tamaño insuficiente: " << len << std::endl;
      return resp;
    }

    // Verificar ID del mensaje
    if (data[0] != MSG_ID) {
      std::cerr << "GET_SENSOR_DATA_RESPONSE: ID incorrecto: "
                << static_cast<int>(data[0]) << std::endl;
      return resp;
    }

    // Extraer sessionId (big endian)
    resp.sessionId = (data[1] << 8) | data[2];

    // Extraer status
    resp.status = data[3];

    // Extraer tamaño del payload (big endian)
    uint16_t payloadSize = (data[4] << 8) | data[5];

    // Verificar que haya suficiente datos para el payload
    if (len < 6 + payloadSize) {
      std::cerr << "GET_SENSOR_DATA_RESPONSE: Payload incompleto. Esperado: "
                << (6 + payloadSize) << ", Recibido: " << len << std::endl;
      return resp;
    }

    // Extraer payload
    if (payloadSize > 0) {
      resp.payload.assign(data + 6, data + 6 + payloadSize);
    }

    return resp;
  }
};

#endif