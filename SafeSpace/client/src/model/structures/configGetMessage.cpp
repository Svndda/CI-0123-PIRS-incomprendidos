#include "configGetMessage.h"

// Getters
uint8_t ConfigGetMessage::getMsgId() const { return this->msgId; }
uint8_t ConfigGetMessage::getSensorId() const { return this->sensorId; }
uint8_t ConfigGetMessage::getParamId() const { return this->paramId; }

// Setters
void ConfigGetMessage::setMsgId(const uint8_t newMsgId) { this->msgId = newMsgId; }
void ConfigGetMessage::setSensorId(const uint8_t newSensorId) { this->sensorId = newSensorId; }
void ConfigGetMessage::setParamId(const uint8_t newParamId) { this->paramId = newParamId; }

// Operador ==
bool ConfigGetMessage::operator==(const ConfigGetMessage& other) const {
    return msgId == other.msgId &&
           sensorId == other.sensorId &&
           paramId == other.paramId;
}

// Operador =
ConfigGetMessage& ConfigGetMessage::operator=(const ConfigGetMessage& other) {
    if (this != &other) {
        msgId = other.msgId;
        sensorId = other.sensorId;
        paramId = other.paramId;
    }
    return *this;
}

// Operador <<
std::ostream& operator<<(std::ostream& os, const ConfigGetMessage& msg) {
    os << "ConfigGetMessage { "
       << "msgId: " << static_cast<int>(msg.msgId) << ", "
       << "sensorId: " << static_cast<int>(msg.sensorId) << ", "
       << "paramId: " << static_cast<int>(msg.paramId)
       << " }";
    return os;
}
