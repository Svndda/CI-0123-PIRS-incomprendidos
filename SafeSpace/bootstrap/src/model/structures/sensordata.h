#ifndef SENSORDATA_H
#define SENSORDATA_H

class SensorData {
public:
  float distance;
  float temperature;
  float pressure;
  float altitude;
  float sealevelPressure;
  float realAltitude;
  
  // Default constructor
  SensorData() noexcept
      : distance(0.0f), temperature(0.0f), pressure(0.0f),
      altitude(0.0f), sealevelPressure(0.0f), realAltitude(0.0f) {}
  
  // Constructor with parameters
  SensorData(float dist, float temp, float press, float alt,
             float seaPress, float realAlt) noexcept
      : distance(dist), temperature(temp), pressure(press),
      altitude(alt), sealevelPressure(seaPress), realAltitude(realAlt) {}
  
  // Explicitly default all copy/move semantics
  SensorData(const SensorData&) noexcept = default;
  SensorData& operator=(const SensorData&) noexcept = default;
  SensorData(SensorData&&) noexcept = default;
  SensorData& operator=(SensorData&&) noexcept = default;
};

#endif // SENSORDATA_H
