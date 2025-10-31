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
  uint8_t  msgId;            ///< 0x42 = SENSOR_DATA
  float    distance;         ///< Distance in centimeters
  float    temperature;      ///< Temperature in Celsius
  float    pressure;         ///< Atmospheric pressure in Pascals
  float    altitude;         ///< Altitude in meters
  float    sealevelPressure; ///< Sea-level adjusted pressure in Pascals
  float    realAltitude;     ///< Real measured altitude in meters
};

#pragma pack(pop)
