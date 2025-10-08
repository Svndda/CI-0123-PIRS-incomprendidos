#include "alertMessage.h"

    uint8_t AlertMessage::getMsgId() const { return this->msgId; }
    uint8_t AlertMessage::getCode() const { return this->alertCode; }
    uint32_t AlertMessage::getValue() const { return this->alertValue; }

    void AlertMessage::setMsgId(const uint8_t newMsgId){
        this->msgId = newMsgId;
    };

    void AlertMessage::setAlertCode(const uint8_t newAlertCode){
        this->alertCode = newAlertCode;
    };

    void AlertMessage::setAlertValue(const uint32_t newAlertValue){
        this->alertValue = newAlertValue;
    };

    // Igualdad
    bool AlertMessage::operator==(const AlertMessage& other) const {
        return msgId == other.msgId && alertCode == other.alertCode &&
                alertValue == other.alertValue;
    }

    // Asignación
    AlertMessage& AlertMessage::operator=(const AlertMessage& other) {
        if (this != &other) {
            msgId = other.msgId;
            alertCode = other.alertCode;
            alertValue = other.alertValue;
        }
        return *this;
    }

    // Inserción
    std::ostream& operator<<(std::ostream& os, const AlertMessage& msg) {
        os  << "AlertMessage { "
            << "msgId: " << static_cast<int>(msg.msgId) << ", "
            << "alertCode: " << static_cast<int>(msg.alertCode) << ", "
            << "alertValue: " << static_cast<int>(msg.alertValue) << ", "
            << " }";
        return os;
    }  