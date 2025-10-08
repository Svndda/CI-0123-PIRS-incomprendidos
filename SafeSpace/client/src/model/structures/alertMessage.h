#ifndef ALERTMESSAGE_H
#define ALERTMESSAGE_H

#include <cstdint>
#include <iostream>

class AlertMessage {
 private:
    uint8_t msgId;   
    uint8_t alertCode;
    uint32_t alertValue;
 public:
    AlertMessage(uint8_t msg = 0, uint8_t code = 0, uint32_t value = 0)
        : msgId(msg), alertCode(code), alertValue(value) {}

    uint8_t getMsgId() const {};
    uint8_t getCode() const {};
    uint32_t getValue() const {};

    void setMsgId(const uint8_t newMsgId){};
    void setAlertCode(const uint8_t alertCode){};
    void setAlertValue(const uint32_t alertValue){};

    bool operator==(const AlertMessage& other) const {};
    AlertMessage& operator=(const AlertMessage& other) {};
    friend std::ostream& operator<<(std::ostream& os, const AlertMessage& msg) {};    
};
#endif // ALERTMESSAGE_H