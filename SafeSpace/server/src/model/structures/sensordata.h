#ifndef SENSORDATA_H
#define SENSORDATA_H

class SensorData {
public:
  const float distance;
  const float temperature;
  const float pressure;
  const float altitude;
  const float sealevelPressure;
  const float realAltitude;

public:
  SensorData(const float _dist = 0, const float _temp = 0, const float _press = 0,
             const float _alt = 0, const float _sealPress = 0, const float _realAlt = 0);
};

#endif // SENSORDATA_H