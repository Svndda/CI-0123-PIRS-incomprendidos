#pragma once
#include <cstdint>

// Empaquetado 1-byte alignment: evita relleno entre campos
#pragma pack(push, 1)

/**
 * @struct SensorPacket
 * @brief Represents a telemetry datagram sent from ArduinoNode to SafeSpaceServer.
 *
 * This structure is packed (no padding) to ensure consistent size and binary layout
 * across all communicating nodes. It contains sensor readings encoded as floats.
 */
struct SensorPacket {
  uint8_t  msgId = 0x42;           // 0x42 = SENSOR_DATA
  int16_t  temp_x100;       // temperatura * 100 (network byte order)
  int16_t  hum_x100;        // humedad * 100 (network byte order)
  int16_t  distance_x100;   // distancia * 100 (network byte order)
  int32_t  pressure_pa;     // presión en Pascales (network byte order)
  int16_t  altitude_x100;   // altitud * 100 (network byte order)

  SensorPacket(
    uint16_t temp, uint16_t hum,
    uint16_t distance, int32_t pressure,
    uint16_t altitude) : temp_x100(temp), hum_x100(hum), distance_x100(distance), pressure_pa(pressure), altitude_x100(altitude) {};
  SensorPacket() = default;
};

#pragma pack(pop)
