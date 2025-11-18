#ifndef CONFIGSETMESSAGE_H
#define CONFIGSETMESSAGE_H

#include <cstdint>
#include <iostream>

class ConfigSetMessage {
 private:
    uint8_t msgId;         // Identificador del tipo de mensaje 
    uint8_t sensorId;      // ID del sensor o nodo
    uint8_t paramId;       // ID del parámetro a modificar
    uint32_t paramValue;   // Nuevo valor del parámetro

 public:
    ConfigSetMessage(uint8_t msg = 0, uint8_t sensor = 0, uint8_t param = 0, uint32_t value = 0)
        : msgId(msg), sensorId(sensor), paramId(param), paramValue(value) {}

    // Getters
    uint8_t getMsgId() const;
    uint8_t getSensorId() const;
    uint8_t getParamId() const;
    uint32_t getParamValue() const;

    // Setters
    void setMsgId(const uint8_t newMsgId);
    void setSensorId(const uint8_t newSensorId);
    void setParamId(const uint8_t newParamId);
    void setParamValue(const uint32_t newParamValue);

    // Operadores
    bool operator==(const ConfigSetMessage& other) const;
    ConfigSetMessage& operator=(const ConfigSetMessage& other);
    friend std::ostream& operator<<(std::ostream& os, const ConfigSetMessage& msg);
};

#endif // CONFIGSETMESSAGE_H
