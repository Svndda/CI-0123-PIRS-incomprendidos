#include "sensordata.h"

SensorData::SensorData(
    const float _dist, const float _temp,
    const float _press, const float _alt,
    const float _sealPress, const float _realAlt)
    : distance(_dist), temperature(_temp), pressure(_press),
      altitude(_alt), sealevelPressure(_sealPress), realAltitude(_realAlt) {
}