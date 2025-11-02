#include "sensordata.h"

SensorData::SensorData(
    const int _id, const std::time_t _time,
    const int _dist, const int _mov,
    const int _temp, const int _uv,
    const int _micro , const int _led,
    const int _buzzer, const int _ligth)
    : id(_id), timestamp(_time),
    distance(_dist), movement(_mov),
    temperature(_temp), uv(_uv),
    microphone(_micro), led(_led),
    buzzer(_buzzer), ligth(_ligth) {
  
}
