#include "test_udp_client.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <iomanip>

// Estructuras para DISCOVER 
#pragma pack(push, 1)
struct DiscoverRequest {
    uint8_t msgId;
    uint8_t flags;
};
#pragma pack(pop)

TestUDPClient::TestUDPClient(const std::string& ip, uint16_t port) 
    : serverIp(ip), serverPort(port) {
    udpClient = new UDPClient(ip, port);
    std::cout << " TestUDPClient para " << ip << ":" << port << std::endl;
}

TestUDPClient::~TestUDPClient() {
    if (udpClient) {
        delete udpClient;
    }
}

std::string TestUDPClient::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), 
           password.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

void TestUDPClient::sendDiscover(uint8_t msgId, uint8_t flags) {
    std::cout << " Enviando DISCOVER - msgId: " << static_cast<int>(msgId)
              << ", flags: " << static_cast<int>(flags) << std::endl;

    udpClient->sendMessage(msgId, flags);
}

void TestUDPClient::sendAuthRequest(uint16_t sessionId, const std::string& username, const std::string& password) {
    // Crear AuthRequest usando las nuevas clases
    std::string password_hash = hashPassword(password);
    AuthRequest authReq(sessionId, username, password_hash);
    
    // Obtener buffer serializado
    auto buffer = authReq.toBuffer();

    std::cout << " Enviando AUTH_REQUEST - sessionId: " << sessionId
              << ", usuario: '" << username << "'" << std::endl;
    std::cout << "   Password hash: " << password_hash.substr(0, 16) << "..." << std::endl;

    // Enviar buffer
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    ssize_t sent = sendto(udpClient->getSocketFd(), 
                         buffer.data(), buffer.size(), 0,
                         reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (sent < 0) {
        std::cerr << " Error enviando AUTH_REQUEST: " << strerror(errno) << std::endl;
    } else {
        std::cout << " AUTH_REQUEST enviado (" << sent << " bytes)" << std::endl;
    }
}