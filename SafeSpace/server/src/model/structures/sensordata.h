#ifndef SENSORDATA_H
#define SENSORDATA_H

class SensorData {
public:
  const int distance;
  const int movement;
  const int temperature;
  const int uv;
  const int microphone;
  const int led;
  const int buzzer;
  const int ligth;
public:
  SensorData(const int _dist = -1, const int _mov = -1, const int _temp = -1,
             const int _uv = -1, const int _micro = -1, const int _led = -1,
             const int _buzzer = -1, const int _ligth = -1);
};

#endif // SENSORDATA_H
