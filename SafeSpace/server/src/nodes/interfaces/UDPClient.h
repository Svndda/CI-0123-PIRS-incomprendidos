#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

/**
 * @class UDPClient
 * @brief A simple UDP client for sending and receiving messages.
 */
class UDPClient {
    public:
    /**
     * @brief Construct a new UDPClient object
     * 
     * @param serverIp for the server
     * @param serverPort for the server
     */
        UDPClient(const std::string& serverIp, uint16_t serverPort);
        /**
         * @brief Destroy the UDPClient object
         * 
         */
        ~UDPClient();
        /**
         * @brief Sends a message to the server.
         * 
         * @param msgId    The ID of the message to send.
         * @param flags    Flags to include with the message.
         * @return true    If the message was sent successfully.
         * @return false   If there was an error sending the message.
         */
        void sendMessage(uint8_t msgId, uint8_t flags);
        /**
         * @brief Receives a message from the server.
         * 
         * @return true    If the message was received successfully.
         * @return false   If there was an error receiving the message.
         */
        void closeSocket();

        int getSocketFd();
    private:
        int sockfd{-1};
        std::string serverIp;
        uint16_t serverPort;
};