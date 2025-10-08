#include "sensorDataMessage.h"

    uint8_t SensorDataMessage::getMsgId() const {return this->msgId;}
    uint8_t SensorDataMessage::getDataId() const {return this->dataId;}
    uint8_t SensorDataMessage::getSensorId() const {return this->sensorId;}
    uint64_t SensorDataMessage::getSensorData() const {return this->sensorData;}

    void SensorDataMessage::setMsgId(const uint8_t newMsgId){
        this->msgId = newMsgId;
    };

    void SensorDataMessage::setDataId(const uint8_t newDataId){
        this->dataId = newDataId;
    };

    void SensorDataMessage::setSensorId(const uint8_t newSensorId){
        this->sensorId = newSensorId;
    };

    void SensorDataMessage::setSensorData(const uint64_t newSensorData){
        this->sensorId = newSensorData;
    };

    // Igualdad
    bool SensorDataMessage::operator==(const SensorDataMessage& other) const {
        return msgId == other.msgId &&
               dataId == other.dataId &&
               sensorId == other.sensorId &&
               sensorData == other.sensorData;
    }

    // Asignación
    SensorDataMessage& SensorDataMessage::operator=(const SensorDataMessage& other) {
        if (this != &other) {
            msgId = other.msgId;
            dataId = other.dataId;
            sensorId = other.sensorId;
            sensorData = other.sensorData;
        }
        return *this;
    }

    // Inserción
    std::ostream& operator<<(std::ostream& os, const SensorDataMessage& msg) {
        os << "SensorDataMessage { "
           << "msgId: " << static_cast<int>(msg.msgId) << ", "
           << "dataId: " << static_cast<int>(msg.dataId) << ", "
           << "sensorId: " << static_cast<int>(msg.sensorId) << ", "
           << "sensorData: " << msg.sensorData
           << " }";
        return os;
    }