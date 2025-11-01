#include "UDPClient.h"
#include "../../model/structures/DiscoverRequest.h"
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <cstring>

UDPClient::UDPClient(const std::string& serverIp, uint16_t serverPort) 
    : serverIp(serverIp), serverPort(serverPort) {
    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("[UDPClient] Failed to create socket");
    }
}
UDPClient::~UDPClient() {
    closeSocket();
}

int UDPClient::getSocketFd() {
    return this->sockfd;
}

void UDPClient::closeSocket() {
    if (sockfd > 0) {
        close(sockfd);
        sockfd = -1;
        std::cout << "[UDPClient] Socket closed." << std::endl;
    }
}

void UDPClient::sendRaw(const void* data, size_t length) {
    if (sockfd < 0) {
        throw std::runtime_error("[UDPClient] Socket is not initialized");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    ssize_t sent = ::sendto(sockfd, data, static_cast<socklen_t>(length), 0,
                            reinterpret_cast<sockaddr*>(&serverAddr),
                            sizeof(serverAddr));

    if (sent < 0) {
        throw std::runtime_error("[UDPClient] Failed to send raw data: " +
                                 std::string(std::strerror(errno)));
    } else if (static_cast<size_t>(sent) != length) {
        std::cerr << "[UDPClient] Warning: partial send (" << sent
                  << " / " << length << " bytes)" << std::endl;
    } else {
        std::cout << "[UDPClient] Sent " << sent << " bytes to "
                  << serverIp << ":" << serverPort << std::endl;
    }
}

void UDPClient::sendMessage(uint8_t msgId, uint8_t flags) {
    if(sockfd < 0) {
        throw std::runtime_error("[UDPClient] Socket is not initialized");
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

    DiscoverRequest request(msgId, flags);
    auto payload = request.toBytes();

    std::cout << "[UDPClient] Sending to " << serverIp << ":" << serverPort
              << " -> " << request << std::endl;

    ssize_t sent = sendto(sockfd,  // Cambio: sock -> sockfd
    payload.data(),
    payload.size(),
    0,
    reinterpret_cast<sockaddr*>(&serverAddr),
    sizeof(serverAddr)
    );

    if(sent < 0) {
        throw std::runtime_error("[UDPClient] Failed to send message");
    }

    std::cout << "[UDPClient] Datagram sent successfully (" 
              << sent << " bytes)" << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
