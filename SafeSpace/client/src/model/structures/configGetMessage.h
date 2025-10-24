#ifndef CONFIGGETMESSAGE_H
#define CONFIGGETMESSAGE_H

#include <cstdint>
#include <iostream>

class ConfigGetMessage {
 private:
    uint8_t msgId;       // Identificador del tipo de mensaje 
    uint8_t sensorId;    // ID del sensor o nodo
    uint8_t paramId;     // ID del parámetro a consultar

 public:
    ConfigGetMessage(uint8_t msg = 0, uint8_t sensor = 0, uint8_t param = 0)
        : msgId(msg), sensorId(sensor), paramId(param) {}

    // Getters
    uint8_t getMsgId() const;
    uint8_t getSensorId() const;
    uint8_t getParamId() const;

    // Setters
    void setMsgId(const uint8_t newMsgId);
    void setSensorId(const uint8_t newSensorId);
    void setParamId(const uint8_t newParamId);

    // Operadores
    bool operator==(const ConfigGetMessage& other) const;
    ConfigGetMessage& operator=(const ConfigGetMessage& other);
    friend std::ostream& operator<<(std::ostream& os, const ConfigGetMessage& msg);
};

#endif // CONFIGGETMESSAGE_H
