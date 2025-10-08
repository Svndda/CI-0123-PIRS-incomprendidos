#ifndef SENSORDATAMSG_H
#define SENSORDATAMSG_H

#include <cstdint>
#include <iostream>

class SensorDataMessage {
 private:
    uint8_t msgId;
    uint8_t dataId;
    uint8_t sensorId;
    uint64_t sensorData;
 public:
    SensorDataMessage(uint8_t msg = 0, uint8_t data = 0, uint8_t sensor = 0,
            uint64_t value = 0) : msgId(msg), dataId(data), sensorId(sensor),
            sensorData(value) {}

    uint8_t getMsgId() const {};
    uint8_t getDataId() const {};
    uint8_t getSensorId() const {};
    uint64_t getSensorData() const {};

    void setMsgId(const uint8_t newMsgId) {};
    void setDataId(const uint8_t setDataId) {};
    void setSensorId(const uint8_t setSensorId) {};
    void setSensorData(const uint64_t setSensorData) {};

    bool operator==(const SensorDataMessage& other) const {};
    SensorDataMessage& operator=(const SensorDataMessage& other) {};
    friend std::ostream& operator<<(std::ostream& os, const SensorDataMessage& msg) {};
};

#endif // SENSORDATAMSG_H