#include "ProxyNode.h"
#include "../../model/structures/DiscoverRequest.h"
#include "../../model/structures/DiscoverResponse.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <vector>

#include "authenticationrequest.h"
#include "authenticationresponse.h"
#include "connectrequest.h"

const size_t BUFFER_SIZE = 2048;

ProxyNode::ProxyNode(const std::string& ip, uint16_t proxyPort, const std::string& authServerIp,
        uint16_t authServerPort) : UDPServer(ip, proxyPort, BUFFER_SIZE),
        authClient(nullptr), authServerIp(authServerIp),
        authServerPort(authServerPort), listening(false) {

    try { // Crea cliente UDP para comunicarse con el servidor de autenticación.
        authClient = new UDPClient(authServerIp, authServerPort);

        std::cout << "[ProxyNode] Initialized on port " << proxyPort 
                  << ", forwarding to auth server " << authServerIp
                  << ":" << authServerPort << std::endl;

    } catch(const std::exception& e) {
        std::cerr << "[ProxyNode] Failed to create auth client: " << e.what() 
                << std::endl;
        throw;
    }
}

ProxyNode::~ProxyNode() {
    std::cout << "[ProxyNode] Shutting down..." << std::endl;
    
    // Detiene el hilo de escucha
    listening.store(false);
    
    // Espera a que el hilo termine
    if (listenerThread.joinable()) {
        std::cout << "[ProxyNode] Waiting for listener thread to finish..." 
                << std::endl;
        listenerThread.join();
    }
    
    // Liberar memoria del cliente
    if (authClient != nullptr) {
        delete authClient;
        authClient = nullptr;
        std::cout << "[ProxyNode] Auth client destroyed" << std::endl;
    }
    
    std::cout << "[ProxyNode] Shutdown complete" << std::endl;
}

void ProxyNode::start() {
    if (authClient == nullptr) {
        throw std::runtime_error("[ProxyNode] Cannot start: auth client not initialized");
    }
    
    // Iniciar hilo para escuchar respuestas del servidor de autenticación
    listening.store(true);
    listenerThread = std::thread(&ProxyNode::listenAuthServerResponses, this);
    
    std::cout << "[ProxyNode] Auth server listener thread started"
            << std::endl;
    std::cout << "[ProxyNode] Starting proxy service (blocking mode)..." 
            << std::endl;
    
    // Ejecutar servidor en modo bloqueante (loop principal)
    serveBlocking();
    
    // Cuando serveBlocking termina, detiene el listener
    listening.store(false);
}

void ProxyNode::onReceive(const sockaddr_in& peer, const uint8_t* data,
        ssize_t len, std::string& out_response) {

    std::string peerStr = sockaddrToString(peer);
    std::cout << "[ProxyNode] Received " << len << " bytes from client " 
              << peerStr << std::endl;

    
    if (len == 2) {  // Verificar si es un mensaje de DISCOVER (2 bytes)
        std::array<uint8_t, 2> payload = { data[0], data[1] };
        DiscoverRequest req = DiscoverRequest::fromBytes(payload);
        uint8_t msgId = req.msgId();
        uint8_t flags = req.rflags();
        
        std::cout << "[ProxyNode] DISCOVER request msgId=" 
                << static_cast<int>(msgId) 
                << " flags=" << static_cast<int>(flags)
                << " from " << peerStr << std::endl;

        
        {  // Guardar información del cliente para responder después
            std::lock_guard<std::mutex> lock(clientsMutex);
            pendingClients[msgId] = {peer, msgId};
            std::cout << "[ProxyNode] Stored client info for msgId=" 
                    << static_cast<int>(msgId) << std::endl;
        }

        
        try {  // Reenviar al servidor de autenticación
            forwardToAuthServer(data, static_cast<size_t>(len));
            std::cout << "[ProxyNode] Successfully forwarded DISCOVER msgId=" 
                    << static_cast<int>(msgId) << " to auth server" << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "[ProxyNode] ERROR forwarding to auth server: " 
                    << e.what() << std::endl;
           
            // Limpiar el cliente pendiente si falla el reenvío
            std::lock_guard<std::mutex> lock(clientsMutex);
            pendingClients.erase(msgId);
        }

        // No responder inmediatamente al cliente
        out_response.clear();
        return;

    }

    if (len == 50) {  // AUTHENTICATION_REQUEST messages are 50 bytes

      // Copiar los 50 bytes recibidos a un array para deserialización
      std::array<uint8_t, 50> payload{};
      std::memcpy(payload.data(), data, 50);

      // Reconstruir AuthRequest a partir del buffer
      AuthRequest req;
      req.setSessionId(static_cast<uint16_t>(payload[0]) << 8 | payload[1]);

      // Copiar username (16 bytes)
      std::string username(reinterpret_cast<char*>(payload.data() + 2), AuthRequest::USERNAME_SIZE);
      req.setUsername(username);

      // Copiar password (32 bytes)
      std::string password(reinterpret_cast<char*>(payload.data() + 2 + AuthRequest::USERNAME_SIZE), AuthRequest::PASSWORD_SIZE);
      req.setPassword(password);

      const uint16_t sessionId = req.getSessionId();
      const std::string& user = req.getUsername();
      const std::string& pass = req.getPassword();

      std::cout << "[ProxyNode] AUTHENTICATION_REQUEST sessionId=" << sessionId
                << " username=" << user
                << " password=" << std::string(pass.size(), '*')
                << " from " << peerStr << std::endl;

      // Guardar información del cliente para responder después
      {
        std::lock_guard<std::mutex> lock(clientsMutex);
        pendingClients[sessionId] = {peer, sessionId};
        std::cout << "[ProxyNode] Stored client info for sessionId=" << sessionId << std::endl;
      }

      // Reenviar al servidor de autenticación
      try {
        forwardToAuthServer(data, static_cast<size_t>(len));
        std::cout << "[ProxyNode] Successfully forwarded AUTHENTICATION_REQUEST sessionId="
                  << sessionId << " to auth server" << std::endl;
      } catch (const std::exception& e) {
        std::cerr << "[ProxyNode] ERROR forwarding to auth server: " << e.what() << std::endl;

        // Limpiar cliente pendiente si falla el reenvío
        std::lock_guard<std::mutex> lock(clientsMutex);
        pendingClients.erase(sessionId);
      }

      // No responder inmediatamente al cliente
      out_response.clear();
      return;
    }

    // Si no es un mensaje conocido, hacer echo por defecto
    std::cout << "[ProxyNode] Unknown message format (" << len 
            << " bytes), using default behavior" << std::endl;
    UDPServer::onReceive(peer, data, len, out_response);
}

void ProxyNode::forwardToAuthServer(const uint8_t* data, size_t len) {
    if (authClient == nullptr) {
        throw std::runtime_error("[ProxyNode] Auth client not initialized");
    }

    if (data == nullptr || len == 0) {
        throw std::runtime_error("[ProxyNode] Invalid data to forward");
    }

    // Dirección de auth server
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(authServerPort);
    
    if (inet_aton(authServerIp.c_str(), &serverAddr.sin_addr) == 0) {
        throw std::runtime_error("[ProxyNode] Invalid auth server IP: " 
                + authServerIp);
    }

    ssize_t sent = ::sendto(authClient->getSocketFd(), data, len, 0,
            reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr));

    if (sent < 0) {
        throw std::runtime_error(std::string("[ProxyNode] sendto auth server failed: ") 
                + std::strerror(errno));
    }
    
    if (static_cast<size_t>(sent) != len) {
        throw std::runtime_error("[ProxyNode] Incomplete send to auth server");
    }
}

void ProxyNode::listenAuthServerResponses() {
    std::cout << "[ProxyNode] Auth server response listener thread started (ID: " 
              << std::this_thread::get_id() << ")" << std::endl;
    
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    
    while (listening.load()) {
        sockaddr_in authAddr{};
        socklen_t addrLen = sizeof(authAddr);

        // Configurar timeout para permitir chequeo periódico de listening flag
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        if (setsockopt(authClient->getSocketFd(), SOL_SOCKET, SO_RCVTIMEO, 
                    &tv, sizeof(tv)) < 0) {
            std::cerr << "[ProxyNode] Failed to set socket timeout: " 
                    << std::strerror(errno) << std::endl;
        }

        ssize_t received = ::recvfrom(authClient->getSocketFd(), buffer.data(),
                buffer.size(), 0, reinterpret_cast<sockaddr*>(&authAddr),
                &addrLen);

        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout normal, continuar loop
                continue;
            }
            if (errno == EINTR) {
                // Interrumpido por señal, verificar flag y continuar
                continue;
            }
            std::cerr << "[ProxyNode] recvfrom auth server error: " 
                      << std::strerror(errno) << std::endl;
            continue;
        }

        if (received == 0) {
            continue;
        }

        // Log de recepción desde el servidor de autenticación
        char authIpStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &authAddr.sin_addr, authIpStr, sizeof(authIpStr));
        std::cout << "[ProxyNode] Received " << received 
                  << " bytes from auth server (" << authIpStr 
                  << ":" << ntohs(authAddr.sin_port) << ")" << std::endl;

        // Verificar si es DISCOVER_RESP (4 bytes)
        if (received == 4) {
            std::array<uint8_t, 4> payload = { 
                buffer[0], buffer[1], buffer[2], buffer[3] 
            };
            
            try {
                DiscoverResponse resp = DiscoverResponse::fromBytes(payload);
                uint8_t msgId = resp.msgId();

                std::cout << "[ProxyNode] DISCOVER_RESP msgId=" 
                          << static_cast<int>(msgId) << std::endl;

                // Buscar cliente original
                ClientInfo clientInfo;
                bool found = false;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    auto it = pendingClients.find(msgId);
                    if (it != pendingClients.end()) {
                        clientInfo = it->second;
                        pendingClients.erase(it);
                        found = true;
                    }
                }

                if (found) {
                    // Reenvia respuesta al cliente original
                    try {
                        sendTo(clientInfo.addr, buffer.data(), 
                               static_cast<size_t>(received));
                        std::cout << "[ProxyNode] Successfully forwarded DISCOVER_RESP msgId=" 
                                  << static_cast<int>(msgId) 
                                  << " to original client " 
                                  << sockaddrToString(clientInfo.addr) << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "[ProxyNode] ERROR forwarding response to client: " 
                                  << e.what() << std::endl;
                    }
                } else {
                    std::cout << "[ProxyNode] WARNING: No pending client for msgId=" 
                              << static_cast<int>(msgId) 
                              << " (possibly timeout or duplicate)" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[ProxyNode] Error parsing DISCOVER_RESP: " 
                          << e.what() << std::endl;
            }
        } else if (received == AuthResponse::MESSAGE_SIZE + 3 + AuthResponse::TOKEN_SIZE){
          this->handleAuthResponse(buffer.data(), static_cast<size_t>(received));
        } else {
            std::cout << "[ProxyNode] Received non-standard response from auth server ("
                      << received << " bytes) - ignoring" << std::endl;
        }
    }
    
    std::cout << "[ProxyNode] Auth server response listener thread stopped" 
            << std::endl;
}


void ProxyNode::handleAuthResponse(const uint8_t* buffer, size_t length) {
  // Safecheck
  if (length != AuthResponse::MESSAGE_SIZE + 3 + AuthResponse::TOKEN_SIZE) {
    std::cerr << "[ProxyNode] Invalid AuthResponse length: " << length << std::endl;
    return;
  }

  // bytes copying
  std::array<uint8_t, 51> payload{};
  std::memcpy(payload.data(), buffer, length);

  // AuthResponse reconstruction.
  AuthResponse resp;
  resp.setSessionId(static_cast<uint16_t>(payload[0]) << 8 | payload[1]);
  resp.setStatusCode(payload[2]);

  std::string msg(reinterpret_cast<char*>(payload.data() + 3), AuthResponse::MESSAGE_SIZE);
  resp.setMessage(msg);

  const std::string token(reinterpret_cast<char*>(
    payload.data() + 3 + AuthResponse::MESSAGE_SIZE),
    AuthResponse::TOKEN_SIZE
    );
  resp.setSessionToken(token);

  const uint16_t sessionId = resp.getSessionId();

  // Logging informativo
  std::cout << "[ProxyNode] AUTH_RESPONSE sessionId=" << sessionId
            << " status=" << static_cast<int>(resp.getStatusCode())
            << " token=" << resp.getSessionToken() << std::endl;


  ClientInfo clientInfo;
  bool found = false;

  {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = pendingClients.find(sessionId);
    if (it != pendingClients.end()) {
      clientInfo = it->second;
      // Eliminates the clients from the pending ones.
      pendingClients.erase(it);
      found = true;
    }
  }

  // Sends the response to the client if found.
  if (found) {
      sendTo(clientInfo.addr, payload.data(), payload.size());
      std::cout << "[ProxyNode] Forwarded AUTH_RESPONSE sessionId="
                << sessionId << " to client "
                << sockaddrToString(clientInfo.addr) << std::endl;
  } else {
    std::cout << "[ProxyNode] WARNING: No pending client for AUTH_RESPONSE sessionId="
              << sessionId << std::endl;
  }
}

std::string ProxyNode::sockaddrToString(const sockaddr_in& addr) const {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
    return std::string(ipStr) + ":" + std::to_string(ntohs(addr.sin_port));
}