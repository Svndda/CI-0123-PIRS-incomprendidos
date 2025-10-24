#ifndef TEST_UDP_CLIENT_H
#define TEST_UDP_CLIENT_H

#include "../interfaces/UDPClient.h"
#include "../../model/structures/authenticationrequest.h"
#include <string>
#include <cstdint>
#include <openssl/sha.h>

class TestUDPClient {
private:
    UDPClient* udpClient;
    std::string serverIp;
    uint16_t serverPort;

public:
    TestUDPClient(const std::string& ip, uint16_t port);
    ~TestUDPClient();

    void sendDiscover(uint8_t msgId, uint8_t flags);
    void sendAuthRequest(uint16_t sessionId, const std::string& username, const std::string& password);
    static std::string hashPassword(const std::string& password);
};

#endif