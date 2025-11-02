#ifndef PROXYNODE_H
#define PROXYNODE_H

#include "../interfaces/UDPServer.h"
#include "../interfaces/UDPClient.h"
#include <mutex>
#include <unordered_map>
#include <arpa/inet.h>
#include <thread>
#include <utility>

#include "SensorPacket.h"

/**
 * Nodo Proxy que actuá como servidor para clientes finales y como cliente hacia
 * el nodo de Autenticación.
 */
class ProxyNode: public UDPServer{
 private:

    struct AuthServerInfo{
      UDPClient* client;  //< Client to communicate with the Authentication node.
      std::string ip;     //< Authentication node ip address.
      uint16_t port;      //< Authentication node ipd address port.

      AuthServerInfo(const std::nullptr_t null, std::string  string, uint16_t uint16)
      : client(null), ip(std::move(string)), port(uint16) {};
    } authNode;

    /**
     * @brief Struct that hold the information related to the communication with the Master Server/Node.
     */
    struct MasterServerInfo {
      UDPClient* client;  //< Client to communicate with the Master node.
      std::string ip;     //< Master node ip address.
      uint16_t port;      //< Master node ipd address port.
      MasterServerInfo(const std::nullptr_t null, std::string  string, const uint16_t uint16)
    : client(null), ip(std::move(string)), port(uint16) {};
    } masterNode;

    /**
     * Mapeo de msgId a información del cliente original para retornar
     * respuestas al cliente correcto.
     */
    struct ClientInfo {
        sockaddr_in addr;
        uint8_t msgId;
    };

    std::unordered_map<uint8_t, ClientInfo> pendingClients;
    std::mutex clientsMutex;

    // Tracking de intentos fallidos de autenticación
    std::unordered_map<std::string, int> failedAttempts;  // IP -> count
    std::mutex failedAttemptsMutex;

    // Convierte sockaddr_in para logging.
    std::string sockaddrToString(const sockaddr_in& addr) const;

    // Envía datos crudos al servidor de autenticación.
    void forwardToAuthServer(const uint8_t* data, size_t len);

    void forwardToMasterServer(const uint8_t *data, size_t len);

    // Escucha respuestas del servidor de autenticación
    void listenAuthServerResponses();

    /**
     * @brief Processes an AuthResponse buffer received from the authentication server
     *        and forwards it to the original client if found.
     *
     * @param buffer Pointer to the received data buffer (expected 51 bytes).
     * @param length Length of the buffer (should be exactly 51 bytes for AuthResponse).
     */
    void handleAuthResponse(const uint8_t* buffer, size_t length);
    std::thread listenerThread;
    std::atomic<bool> listening;

    
 public:
    ProxyNode(const std::string& ip, uint16_t proxyPort, const std::string& authServerIp,
            uint16_t authServerPort, const std::string& masterServerIp, uint16_t masterServerPort);
    ~ProxyNode() override;

    // Inicia Proxy
    void start();

 protected:
    /**
     * Maneja mensajes recibidos de clientes finales.
     */
    void onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len,
            std::string& out_response) override;
};

#endif // PROXYNODE_H