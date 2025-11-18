#include "configSetMessage.h"

// Getters
uint8_t ConfigSetMessage::getMsgId() const { return this->msgId; }
uint8_t ConfigSetMessage::getSensorId() const { return this->sensorId; }
uint8_t ConfigSetMessage::getParamId() const { return this->paramId; }
uint32_t ConfigSetMessage::getParamValue() const { return this->paramValue; }

// Setters
void ConfigSetMessage::setMsgId(const uint8_t newMsgId) { this->msgId = newMsgId; }
void ConfigSetMessage::setSensorId(const uint8_t newSensorId) { this->sensorId = newSensorId; }
void ConfigSetMessage::setParamId(const uint8_t newParamId) { this->paramId = newParamId; }
void ConfigSetMessage::setParamValue(const uint32_t newParamValue) { this->paramValue = newParamValue; }

// Operador ==
bool ConfigSetMessage::operator==(const ConfigSetMessage& other) const {
    return msgId == other.msgId &&
           sensorId == other.sensorId &&
           paramId == other.paramId &&
           paramValue == other.paramValue;
}

// Operador =
ConfigSetMessage& ConfigSetMessage::operator=(const ConfigSetMessage& other) {
    if (this != &other) {
        msgId = other.msgId;
        sensorId = other.sensorId;
        paramId = other.paramId;
        paramValue = other.paramValue;
    }
    return *this;
}

// Operador <<
std::ostream& operator<<(std::ostream& os, const ConfigSetMessage& msg) {
    os << "ConfigSetMessage { "
       << "msgId: " << static_cast<int>(msg.msgId) << ", "
       << "sensorId: " << static_cast<int>(msg.sensorId) << ", "
       << "paramId: " << static_cast<int>(msg.paramId) << ", "
       << "paramValue: " << msg.paramValue
       << " }";
    return os;
}
