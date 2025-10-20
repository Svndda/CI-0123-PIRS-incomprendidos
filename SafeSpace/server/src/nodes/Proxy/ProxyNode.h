#ifndef PROXYNODE_H
#define PROXYNODE_H

#include "../interfaces/UDPServer.h"
#include "../interfaces/UDPClient.h"
#include <mutex>
#include <unordered_map>
#include <arpa/inet.h>
#include <thread>

/**
 * Nodo Proxy que actuá como servidor para clientes finales y como cliente hacia
 * el nodo de Autenticación.
 */
class ProxyNode: public UDPServer{
 private:
    // Cliente UDP para comunicarse con servidor de autenticación.
    UDPClient* authClient;
    std::string authServerIp;  // IP del servidor de autenticación
    uint16_t authServerPort;  // Puerto del servidor de autenticación

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

    // Convierte sockaddr_in para logging.
    std::string sockaddrToString(const sockaddr_in& addr) const;

    // Envía datos crudos al servidor de autenticación.
    void forwardToAuthServer(const uint8_t* data, size_t len);

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
    ProxyNode(uint16_t proxyPort, const std::string& authServerIp, 
            uint16_t authServerPort);
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