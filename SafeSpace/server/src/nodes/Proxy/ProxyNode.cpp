#include "ProxyNode.h"
#include "../../common/LogManager.h"
#include "../../model/structures/DiscoverRequest.h"
#include "../../model/structures/DiscoverResponse.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <vector>

#include "authenticationrequest.h"
#include "authenticationresponse.h"
#include "connectrequest.h"
#include "DeleteSensorDataRequest.h"
#include "DeleteSensorDataResponse.h"
#include "GetSensorDataRequest.h"
#include "GetSensorDataResponse.h"
#include "GetSystemUsersRequest.h"
#include "GetSystemUsersResponse.h"
#include "sensordata.h"

const size_t BUFFER_SIZE = 2048;
constexpr uint8_t QUERY_BY_DATE = 0x01;
constexpr uint8_t QUERY_BY_SENSOR = 0x02;
constexpr uint8_t RESPONSE_SENSOR_HISTORY = 0x21;

ProxyNode::ProxyNode(
  const std::string &ip, uint16_t proxyPort,
  const std::string &authServerIp, uint16_t authServerPort,
  const std::string &masterServerIp, uint16_t masterServerPort
) : UDPServer(ip, proxyPort, BUFFER_SIZE),
    authNode(nullptr, authServerIp, authServerPort),
    masterNode(nullptr, masterServerIp, masterServerPort),
    listening(false) {
  try {
    LogManager::instance().enableFileLogging("./build/proxy_node_logs.log");
    LogManager::instance().configureRemote(masterNode.ip, masterNode.port, "ProxyNode");
    
    LogManager::instance().info("ProxyNode configured to forward logs to SafeSpaceServer at " +
                      masterNode.ip + ":" + std::to_string(masterNode.port));
    LogManager::instance().ipAddress("PROXY:" + ip + ":" + std::to_string(proxyPort));
  } catch (const std::exception &ex) {
    LogManager::instance().error(std::string("Failed to configure SafeSpace log forwarding: ") + ex.what());
  }

  try {
    this->authNode.client = new UDPClient(this->authNode.ip, this->authNode.port);
    LogManager::instance().info("Auth client initialized for " + authServerIp + ":" + std::to_string(authServerPort));
  } catch (const std::exception &e) {
    LogManager::instance().error(std::string("Failed to create auth client: ") + e.what());
    throw;
  }

  try {
    this->masterNode.client = new UDPClient(this->masterNode.ip, this->masterNode.port);
    LogManager::instance().info("Master client initialized for " + masterServerIp + ":" + std::to_string(masterServerPort));
  } catch (const std::exception &e) {
    LogManager::instance().error(std::string("Failed to create master client: ") + e.what());
    throw;
  }
}

ProxyNode::~ProxyNode() {
  LogManager::instance().info("ProxyNode shutting down - Security logging ended");
  LogManager::instance().disableFileLogging();
  listening.store(false);

  if (listenerThread.joinable()) {
    listenerThread.join();
  }

  delete this->authNode.client;
  delete this->masterNode.client;

  LogManager::instance().info("ProxyNode shutdown complete.");
}

void ProxyNode::start() {
  // Checks the auth client.
  if (!this->authNode.client) {
    LogManager::instance().error("Cannot start ProxyNode: auth client not initialized");
    throw std::runtime_error("Auth client not initialized");
  }
  // Starts the listener
  listening.store(true);
  listenerThread = std::thread(&ProxyNode::listenAuthServerResponses, this);

  // Logs that the proxy has been initialized.
  LogManager::instance().info("ProxyNode authentication listener started");
  this->serveBlocking();

  listening.store(false);
}

void ProxyNode::onReceive(const sockaddr_in &peer, const uint8_t *data,
                          ssize_t len, std::string &out_response) {
  try {
    uint8_t msgId = data[0];
    if (len == sizeof(ConnectRequest))
      return this->handleConnectRequest(peer, data, len, out_response);

    if ((len == 19 || len == 20) &&
        (data[2] == QUERY_BY_DATE || data[2] == QUERY_BY_SENSOR)) {
      return this->handleSensorQuery(peer, data, len, out_response);
    }

    if (len == 50)
      return this->handleAuthRequest(peer, data, len, out_response);

    if (msgId == 0x90 && len == 21) {
      const auto* pkt = reinterpret_cast<const GetSensorDataRequest*>(data);
      // Obtener IP y puerto del remitente
      char ipbuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

      std::cout << "[SafeSpaceServer] GetSensorDataRequest recibido desde "
                << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;

      try {
        masterNode.client->sendRaw(pkt, sizeof(GetSensorDataRequest));
      } catch (const std::exception& ex) {
        std::cerr << "[SafeSpaceServer] Exception al reenviar SENSOR_PACKET: "
                  << ex.what() << std::endl;
      }

      // Generar respuesta ACK simple al emisor original (Arduino / Intermediario)
      std::string ack = "ACK_SENSOR";
      out_response.assign(ack.begin(), ack.end());
      return;
    }

    if (msgId == 0x94 && len == 21) {
      const auto* pkt = reinterpret_cast<const DeleteSensorDataRequest*>(data);
      // Obtener IP y puerto del remitente
      char ipbuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

      std::cout << "[SafeSpaceServer] GetSensorDataRequest recibido desde "
                << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;

      uint16_t sessionId = pkt->sessionId;

      {
        std::lock_guard<std::mutex> lk(clientsMutex);
        pendingClients[sessionId] = ClientInfo{peer, pkt->MSG_ID};
      }

      try {
        masterNode.client->sendRaw(pkt, sizeof(DeleteSensorDataRequest));
      } catch (const std::exception& ex) {
        std::cerr << "[SafeSpaceServer] Exception al reenviar SENSOR_PACKET: "
                  << ex.what() << std::endl;
      }

      // Generar respuesta ACK simple al emisor original (Arduino / Intermediario)
      std::string ack = "ACK_SENSOR";
      out_response.assign(ack.begin(), ack.end());
      return;
    }

    if (len == sizeof(GetSensorDataResponse)) {
      const auto* pkt = reinterpret_cast<const GetSensorDataResponse*>(data);

      const uint16_t sessionId = pkt->sessionId;

      // Retrieve client pending info
      ClientInfo clientInfo;
      bool found = false;
      {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = pendingClients.find(sessionId);

        if (it != pendingClients.end()) {
          clientInfo = it->second;
          found = true;
        }
      }

      if (!found) {
        this->logger.warning(
          "GetSensorDataResponse received for unknown sessionId=" + std::to_string(sessionId)
        );
        return;
      }

      try {
        this->logger.info(
          "Forwarding GetSensorDataResponse to client sessionId=" + std::to_string(sessionId)
        );

        this->sendTo(clientInfo.addr, data, len);
      } catch (const std::exception &ex) {
        this->logger.error(
          std::string("Failed to forward GetSensorDataResponse to client: ") + ex.what()
        );
        return;
      }
    }

    if (len == sizeof(DeleteSensorDataResponse)) {
      const auto* pkt = reinterpret_cast<const DeleteSensorDataResponse*>(data);

      const uint16_t sessionId = pkt->sessionId;

      // Retrieve client pending info
      ClientInfo clientInfo;
      bool found = false;
      {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = pendingClients.find(sessionId);

        if (it != pendingClients.end()) {
          clientInfo = it->second;
          found = true;
        }
      }

      if (!found) {
        this->logger.warning(
          "DeleteSensorDataResponse received for unknown sessionId=" + std::to_string(sessionId)
        );
        return;
      }

      try {
        this->logger.info(
          "Forwarding DeleteSensorDataResponse to client sessionId=" + std::to_string(sessionId)
        );

        this->sendTo(clientInfo.addr, data, len);
      } catch (const std::exception &ex) {
        this->logger.error(
          std::string("Failed to forward DeleteSensorDataResponse to client: ") + ex.what()
        );
        return;
      }
    }

    if (len == sizeof(SensorData))
      return this->handleSensorData(peer, data, len, out_response);

    if (len == sizeof(GetSystemUsersRequest)) {
      GetSystemUsersRequest req;
      std::memcpy(&req, data, sizeof(req));

      // Convertir a host byte order
      req.toHostOrder();  // O simplemente: req.sessionId = ntohs(req.sessionId);

      uint16_t sessionId = req.sessionId;

      {
        std::lock_guard<std::mutex> lk(clientsMutex);
        pendingClients[sessionId] = ClientInfo{peer, req.msgId};
      }

      // Convertir de vuelta a network byte order para enviar
      req.toNetworkOrder();

      // Forward raw packet to AuthNode
      authNode.client->sendRaw(&req,
        sizeof(req));

      std::cout << "[ProxyNode] Forwarded GetSystemUsersRequest (sessionId="
                << sessionId << ") to AuthNode." << std::endl;
      return;
    }

    if (len >= 5 && data[0] == 'L' && data[1] == 'O' && data[2] == 'G')
      return this->handleLogMessage(peer, data, len);

    this->handleUnknownMessage(peer, data, len, out_response);
  } catch (const std::exception &ex) {
    LogManager::instance().error(std::string("Error handling packet: ") + ex.what());
  }
}

void ProxyNode::handleConnectRequest(const sockaddr_in &peer, const uint8_t *data,
                                     ssize_t len, std::string &out_response) {
  if (len < 6) {
    LogManager::instance().warning("CONNECT_REQUEST too short (" + std::to_string(len) + " bytes)");
    return;
  }

  // Validate identifier
  if (data[0] != ConnectRequest::IDENTIFIER) {
    LogManager::instance().warning("CONNECT_REQUEST invalid identifier: " + std::to_string(data[0]));
    return;
  }

  // Manual deserialization — network order (big endian)
  uint16_t sessionId = (data[1] << 8) | data[2];
  uint16_t sensorId  = (data[3] << 8) | data[4];
  uint8_t flagBits   = data[5];

  LogManager::instance().info("CONNECT_REQUEST received: sessionId=" + std::to_string(sessionId) +
                    ", sensorId=" + std::to_string(sensorId) +
                    ", flags=" + std::to_string(flagBits));

  if (!this->isClientAuthenticated(sessionId)) {
    LogManager::instance().warning("Unauthorized CONNECT_REQUEST (sessionId=" + std::to_string(sessionId) + ")");
    out_response = "UNAUTHORIZED";
    return;
  }

  this->registerSubscriber(peer, sessionId);

  out_response = "SUBSCRIBED_OK";
}

void ProxyNode::handleAuthRequest(const sockaddr_in &peer, const uint8_t *data,
                                  ssize_t len, std::string &out_response) {
  std::array<uint8_t, 50> payload{};
  std::memcpy(payload.data(), data, 50);

  AuthRequest req;
  req.setSessionId((payload[0] << 8) | payload[1]);
  req.setUsername(std::string(reinterpret_cast<const char *>(payload.data() + 2), AuthRequest::USERNAME_SIZE));
  req.setPassword(std::string(reinterpret_cast<const char *>(payload.data() + 18), AuthRequest::PASSWORD_SIZE));

  const uint16_t sessionId = req.getSessionId();
  if (this->isClientAuthenticated(sessionId)) {
    LogManager::instance().info("AUTH_REQUEST ignored: session already authenticated (sessionId=" +
                  std::to_string(sessionId) + ")");

    // We can optionally send back an OK-style auth response here
    AuthResponse autoResp(sessionId, 1, "ALREADY_AUTHENTICATED", "LOCAL_TOKEN");
    auto buffer = autoResp.toBuffer();
    this->sendTo(peer, buffer.data(), buffer.size());
    return;
  }

  this->storePendingClient(sessionId, peer);

  try {
    this->forwardToAuthServer(data, len);
    LogManager::instance().info("Forwarded AUTH_REQUEST for sessionId=" + std::to_string(sessionId));
  } catch (const std::exception &e) {
    LogManager::instance().error(std::string("Error forwarding AUTH_REQUEST: ") + e.what());
    this->removePendingClient(sessionId);
  }
}

void ProxyNode::handleSensorData(const sockaddr_in &peer, const uint8_t *data,
                                 ssize_t len, std::string &out_response) {
  const auto *pkt = reinterpret_cast<const SensorData *>(data);
  LogManager::instance().info(
    "Received SENSOR_DATA: Temp=" + std::to_string(pkt->temperature) +
    " Dist=" + std::to_string(pkt->distance)
    );
  this->broadcastToSubscribers(data, len);
  out_response = "ACK_SENSOR";
}

void ProxyNode::handleSensorQuery(const sockaddr_in &peer, const uint8_t *data,
                                  ssize_t len, std::string &out_response) {
  if (len < 3) {
    LogManager::instance().warning("SENSOR_QUERY too short");
    out_response = "INVALID_QUERY";
    return;
  }

  uint16_t sessionId = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  uint8_t msgType = data[2];

  if (!this->isClientAuthenticated(sessionId)) {
    LogManager::instance().warning("Unauthorized SENSOR_QUERY (sessionId=" + std::to_string(sessionId) + ")");
    out_response = "UNAUTHORIZED";
    return;
  }

  LogManager::instance().info("Forwarding SENSOR_QUERY type=" + std::to_string(msgType) +
                    " from sessionId=" + std::to_string(sessionId) + " to master");

  // Strip the sessionId before forwarding to the master.
  std::vector<uint8_t> payload(data + 2, data + len);

  try {
    this->forwardToMasterServer(payload.data(), payload.size());
  } catch (const std::exception &e) {
    LogManager::instance().error(std::string("Failed to forward query to master: ") + e.what());
    out_response = "FORWARD_ERROR";
    return;
  }

  // Wait for master's response (which should be the StorageNode response).
  std::vector<uint8_t> buffer(65535);
  sockaddr_in src{};
  socklen_t srcLen = sizeof(src);
  struct timeval tv {2, 0};
  setsockopt(this->masterNode.client->getSocketFd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  // Descartar datagramas antiguos y quedarse con la respuesta de historial
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (std::chrono::steady_clock::now() < deadline) {
    ssize_t received = ::recvfrom(
      this->masterNode.client->getSocketFd(),
      buffer.data(),
      buffer.size(),
      0,
      reinterpret_cast<sockaddr*>(&src),
      &srcLen
    );

    if (received <= 0) {
      continue;
    }

    if (buffer[0] == RESPONSE_SENSOR_HISTORY) {
      out_response.assign(reinterpret_cast<const char*>(buffer.data()), received);
      return;
    }
    // Ignorar ACKs u otras respuestas rezagadas.
  }

  LogManager::instance().warning("Timeout waiting Storage response for SENSOR_QUERY");
  out_response = "TIMEOUT";
}

void ProxyNode::handleLogMessage(const sockaddr_in &peer, const uint8_t *data, ssize_t len) {
  uint8_t level = data[3];
  uint8_t nodeLen = data[4];

  if (len < 5 + nodeLen) {
    return;
  }

  std::string node(reinterpret_cast<const char *>(data + 5), nodeLen);
  std::string msg(reinterpret_cast<const char *>(data + 5 + nodeLen), len - 5 - nodeLen);

  LogManager::instance().log(static_cast<LogLevel>(level), "[FROM_" + node + "] " + msg);
}

void ProxyNode::handleUnknownMessage(const sockaddr_in &peer, const uint8_t *data,
                                     ssize_t len, std::string &out_response) {
  LogManager::instance().warning("Unknown message type (" + std::to_string(len) + " bytes)");
  UDPServer::onReceive(peer, data, len, out_response);
}

void ProxyNode::registerSubscriber(const sockaddr_in &addr, uint16_t sessionId) {
  std::lock_guard<std::mutex> lock(this->subscribersMutex);
  this->subscribers[sessionId]= {addr, 0};
}

void ProxyNode::storePendingClient(uint16_t sessionId, const sockaddr_in &addr) {
  std::lock_guard<std::mutex> lock(clientsMutex);
  pendingClients[sessionId] = {addr, sessionId};
}

void ProxyNode::removePendingClient(uint16_t sessionId) {
  std::lock_guard<std::mutex> lock(clientsMutex);
  pendingClients.erase(sessionId);
}

void ProxyNode::forwardToAuthServer(const uint8_t *data, size_t len) {
  if (!this->authNode.client)
    throw std::runtime_error("Auth client not initialized");

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(this->authNode.port);

  if (inet_aton(this->authNode.ip.c_str(), &serverAddr.sin_addr) == 0)
    throw std::runtime_error("Invalid auth server IP: " + this->authNode.ip);

  ssize_t sent = ::sendto(this->authNode.client->getSocketFd(), data, len, 0,
                          reinterpret_cast<const sockaddr *>(&serverAddr), sizeof(serverAddr));

  if (sent < 0 || static_cast<size_t>(sent) != len)
    throw std::runtime_error(std::string("sendto auth server failed: ") + std::strerror(errno));
}


void ProxyNode::forwardToMasterServer(const uint8_t *data, size_t len) {
  if (!this->masterNode.client) {
    throw std::runtime_error("Master client not initialized");

  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(this->masterNode.port);

  if (inet_aton(this->masterNode.ip.c_str(), &serverAddr.sin_addr) == 0) {
    throw std::runtime_error("Invalid master server IP: " + this->masterNode.ip);

  }
  ssize_t sent = ::sendto(this->masterNode.client->getSocketFd(), data, len, 0,
                          reinterpret_cast<const sockaddr *>(&serverAddr), sizeof(serverAddr));

  if (sent < 0 || static_cast<size_t>(sent) != len) {
    throw std::runtime_error(std::string("sendto master server failed: ") + std::strerror(errno));
  }
}

void ProxyNode::listenAuthServerResponses() {
  LogManager::instance().info("Auth server listener thread started (ID=" +
                    std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ")");
  std::vector<uint8_t> buffer(BUFFER_SIZE);

  while (listening.load()) {
    sockaddr_in authAddr{};
    socklen_t addrLen = sizeof(authAddr);

    ssize_t received = this->receiveFromAuthServer(buffer, authAddr, addrLen);
    if (received <= 0) continue;

    this->processAuthServerResponse(buffer, static_cast<size_t>(received), authAddr);
  }

  LogManager::instance().info("Auth server response listener stopped.");
}

ssize_t ProxyNode::receiveFromAuthServer(std::vector<uint8_t>& buffer, sockaddr_in& authAddr, socklen_t& addrLen) {
  struct timeval tv {1, 0};
  setsockopt(this->authNode.client->getSocketFd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  ssize_t received = ::recvfrom(
    this->authNode.client->getSocketFd(),
    buffer.data(),
    buffer.size(),
    0,
    reinterpret_cast<sockaddr*>(&authAddr),
    &addrLen
  );

  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return -1;
    LogManager::instance().error(std::string("recvfrom() failed: ") + std::strerror(errno));
    return -1;
  }

  return received;
}

void ProxyNode::processAuthServerResponse(const std::vector<uint8_t>& buffer, size_t length,
                                          const sockaddr_in&) {
  if (length == AuthResponse::MESSAGE_SIZE + 3 + AuthResponse::TOKEN_SIZE) {
    this->handleAuthResponse(buffer.data(), length);
  } else if (length == sizeof(GetSystemUsersResponse)) {
    this->handleGetSystemUsersResponse(buffer.data(), length);
  } else {
    LogManager::instance().warning("Received non-standard response (" + std::to_string(length) + " bytes).");
  }
}

void ProxyNode::broadcastToSubscribers(const uint8_t* data, size_t len) {
  std::lock_guard<std::mutex> lock(subscribersMutex);
  if (subscribers.empty()) {
    LogManager::instance().info("No active subscribers to broadcast.");
    return;
  }

  LogManager::instance().info("Broadcasting SensorData to " + std::to_string(subscribers.size()) + " subscribers.");
  for (const auto& sub : subscribers) {
    try {
      this->sendTo(sub.second.addr, data, len);
      LogManager::instance().info("Data sent to " + sockaddrToString(sub.second.addr));
    } catch (const std::exception& e) {
      LogManager::instance().error(std::string("Failed to send to subscriber: ") + e.what());
    }
  }
}

void ProxyNode::handleAuthResponse(const uint8_t *buffer, size_t length) {
  // Validate expected size
  if (length != AuthResponse::MESSAGE_SIZE + 3 + AuthResponse::TOKEN_SIZE) {
    LogManager::instance().warning("Invalid AuthResponse length: " + std::to_string(length));
    return;
  }

  // Deserialize raw buffer into AuthResponse
  std::array<uint8_t, 128> payload{};
  std::memcpy(payload.data(), buffer, length);

  AuthResponse resp;
  resp.setSessionId((payload[0] << 8) | payload[1]);
  resp.setStatusCode(payload[2]);
  resp.setMessage(
    std::string(reinterpret_cast<const char*>(payload.data() + 3),
                AuthResponse::MESSAGE_SIZE)
  );
  resp.setSessionToken(
    std::string(reinterpret_cast<const char*>(payload.data() + 3 + AuthResponse::MESSAGE_SIZE),
                AuthResponse::TOKEN_SIZE)
  );

  const uint16_t sessionId = resp.getSessionId();

  // Retrieve client pending info
  ClientInfo clientInfo;
  bool found = false;
  {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = pendingClients.find(sessionId);

    if (it != pendingClients.end()) {
      clientInfo = it->second;
      found = true;
    }
  }

  if (!found) {
    this->logger.warning(
      "AuthResponse received for unknown sessionId=" + std::to_string(sessionId)
    );
    return;
  }

  // Forward raw AuthResponse back to the appropriate client
  try {
    this->logger.info(
      "Forwarding AuthResponse to client sessionId=" + std::to_string(sessionId)
    );

    this->sendTo(clientInfo.addr, buffer, length);
  } catch (const std::exception &ex) {
    this->logger.error(
      std::string("Failed to forward AuthResponse to client: ") + ex.what()
    );
    return;
  }

  // Register authenticated client
  if (resp.getStatusCode() == 1) {
    LogManager::instance().info("Authentication successful for sessionId " + std::to_string(sessionId));
    if (found) {
      try {
        this->registerAuthenticatedClient(clientInfo.addr, sessionId);
        LogManager::instance().info("Client authenticated and registered for CONNECT_REQUEST (sessionId=" +
                          std::to_string(sessionId) + ")");

        // Remove from pending clients after response is delivered
        {
          std::lock_guard<std::mutex> lock(clientsMutex);
          pendingClients.erase(sessionId);
        }

      } catch (const std::exception& ex) {
        LogManager::instance().error(std::string("Failed to register authenticated client (sessionId=") +
                          std::to_string(sessionId) + "): " + ex.what());
      }
    }
  } else {
    LogManager::instance().warning("Authentication failed for sessionId " + std::to_string(sessionId));
  }

  if (found) {
    this->sendTo(clientInfo.addr, payload.data(), payload.size());
    LogManager::instance().info("Forwarded AUTH_RESPONSE sessionId=" + std::to_string(sessionId));
  } else {
    LogManager::instance().warning("No pending client for AUTH_RESPONSE sessionId=" + std::to_string(sessionId));
  }
}
    
void ProxyNode::handleGetSystemUsersResponse(const uint8_t *buffer, size_t length) {
  if (length != sizeof(GetSystemUsersResponse)) {
    this->logger.warning("Tamaño incorrecto para GetSystemUsersResponse");
    return;
  }

  // CORRECCIÓN: Copiar la estructura y CONVERTIR a host byte order
  GetSystemUsersResponse resp;
  std::memcpy(&resp, buffer, sizeof(resp));

  resp.sessionId = ntohs(resp.sessionId);
  resp.totalUsers = ntohs(resp.totalUsers);
  resp.currentIndex = ntohs(resp.currentIndex);

  uint16_t sessionId = resp.sessionId;  // Ahora sí es 1001
  uint16_t currentIndex = resp.currentIndex;
  uint16_t totalUsers = resp.totalUsers;

  // Buscar cliente usando el sessionId de la respuesta
  ClientInfo clientInfo;
  bool found = false;
  {
    std::lock_guard<std::mutex> lk(clientsMutex);
    auto it = pendingClients.find(sessionId);
    if (it != pendingClients.end()) {
      clientInfo = it->second;
      found = true;

      // Si es el último paquete, eliminar del mapa
      if (currentIndex + 1 == totalUsers) {
        pendingClients.erase(sessionId);
        this->logger.info("Respuesta completa de usuarios recibida para sessionId=" +
                         std::to_string(sessionId));
      }
    }
  }

  if (found) {
    // Reenviar al cliente original

    sendTo(clientInfo.addr, buffer, length);

    this->logger.info("Reenviado GetSystemUsersResponse ["
                     + std::to_string(currentIndex + 1) + "/"
                     + std::to_string(totalUsers) + "] para sessionId="
                     + std::to_string(sessionId));
  } else {
    this->logger.warning("No hay cliente pendiente para sessionId="
                        + std::to_string(sessionId));
  }
}

void ProxyNode::registerAuthenticatedClient(const sockaddr_in &addr, uint16_t sessionId) {
  std::lock_guard<std::mutex> lock(authenticatedMutex);
  // If it's already there, do nothing
  if (this->authenticatedClients.find(sessionId) != this->authenticatedClients.end())
    return;

  this->authenticatedClients[sessionId] = ClientInfo{addr, 0};
  LogManager::instance().info("Registered authenticated client (sessionId=" + std::to_string(sessionId) + ")");
}

bool ProxyNode::isClientAuthenticated(uint16_t sessionId) {
  std::lock_guard<std::mutex> lock(authenticatedMutex);
  return this->authenticatedClients.find(sessionId) != this->authenticatedClients.end();
}

std::string ProxyNode::sockaddrToString(const sockaddr_in &addr) const {
  char ipStr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
  return std::string(ipStr) + ":" + std::to_string(ntohs(addr.sin_port));
}
