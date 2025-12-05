#include "SafeSpaceServer.h"
#include "../model//structures/DiscoverRequest.h"
#include "../model//structures/DiscoverResponse.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <algorithm>

#include "sensordata.h"
#include "../../../common/LogManager.h"
#include "SensorPacket.h"
#include "Storage/StorageNode.h"
#include <GetSensorDataRequest.h>
#include <DeleteSensorDataRequest.h>

enum class LogLevel;

void SafeSpaceServer::runInternalTests() {
  std::cout << "[SafeSpaceServer] Running internal StorageNode tests...\n";

  // -------------------------------------------------------------------
  // 1. Build a dummy token (16 bytes)
  // -------------------------------------------------------------------
  uint8_t rawToken[16];
  for (int i = 0; i < 16; ++i) {
    rawToken[i] = i;  // predictable test token
  }
  Token16 token(rawToken);

  uint16_t testSensorId = 0;  // arbitrary test sensor

  // -------------------------------------------------------------------
  // 2. GET SENSOR DATA TEST
  // -------------------------------------------------------------------
  std::cout << "[Test] Sending GET_SENSOR_DATA_REQUEST...\n";
  GetSensorDataResponse getResp = sendGetSensorData(testSensorId, token);

  std::cout << "  Status: " << (int)getResp.status << "\n";
  std::cout << "  Payload size: " << getResp.payload.size() << "\n";

  if (!getResp.payload.empty()) {
    std::cout << "  Payload (hex): ";
    for (uint8_t b : getResp.payload) {
      printf("%02X ", b);
    }
    std::cout << "\n";
  }

  // // -------------------------------------------------------------------
  // // 4. DELETE SENSOR DATA TEST
  // // -------------------------------------------------------------------
  // std::cout << "[Test] Sending DELETE_SENSOR_DATA_REQUEST...\n";
  // DeleteSensorDataResponse delResp =
  //   this->sendDeleteSensorData(testSensorId, token);
  //
  // std::cout << "  Status: " << (int)delResp.status << "\n";
  //
  // std::cout << "[SafeSpaceServer] StorageNode tests completed.\n";
}

SafeSpaceServer::SafeSpaceServer(const std::string& ip, const uint16_t port,
                                 const std::string& storageIp, const uint16_t storagePort,
                                 const std::string& eventsIp, const uint16_t eventsPort,
                                 const std::string& proxyIp, const uint16_t proxyPort)
  : UDPServer(ip, port, 2048)
  , storageNode(nullptr, storageIp, storagePort)
  , eventsNode(nullptr, eventsIp, eventsPort)
  , proxyNode(nullptr, proxyIp, proxyPort) {
  std::cout << "SafeSpaceServer: initialized on port " << port << std::endl;
  
  // Configurar LogManager para enviar logs a CriticalEventsNode
  auto& logger = LogManager::instance();
  logger.enableFileLogging("./build/server_security_audit.log");
  logger.configureRemote(eventsIp, eventsPort, "SafeSpaceServer");
  logger.info("SafeSpaceServer logging system initialized");
  logger.ipAddress("MASTER:" + ip + ":" + std::to_string(port));

  try {
    if (!storageNode.client) {
      storageNode.client = new UDPClient(storageNode.ip, storageNode.port);
      std::cout << "[SafeSpaceServer] Created Storage UDP client to "
                << storageNode.ip << ":" << storageNode.port << std::endl;
    }

    if (!proxyNode.client) {
      proxyNode.client = new UDPClient(proxyNode.ip, proxyNode.port);
      std::cout << "[SafeSpaceServer] Created Proxy UDP client to "
                << proxyNode.ip << ":" << proxyNode.port << std::endl;
    }
    std::cout << "SafeSpaceServer: started CriticalEventsNode on port " << (port + 1) << std::endl;
    this->runInternalTests();
  } catch (const std::exception &ex) {
    std::cerr << "SafeSpaceServer: failed to initialize UDP clients: " << ex.what() << std::endl;
  }
}

SafeSpaceServer::~SafeSpaceServer() {
  // Gracefully stop master-side logging
  auto& logger = LogManager::instance();
  logger.info("Server shutting down - Security logging ended");
  logger.disableFileLogging();
}

sockaddr_in SafeSpaceServer::makeSockaddr(const std::string& ip, uint16_t port) {
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  if (::inet_aton(ip.c_str(), &a.sin_addr) == 0) {
    throw std::runtime_error("Invalid IPv4 address: " + ip);
  }
  return a;
}

void SafeSpaceServer::addDiscoverTarget(const std::string& ip, uint16_t port) {
  sockaddr_in a = makeSockaddr(ip, port);
  std::lock_guard<std::mutex> lg(targetsMutex_);
  discoverTargets_.push_back(a);
  std::cout << "SafeSpaceServer: added discover target " << ip << ":" << port << std::endl;
}

void SafeSpaceServer::clearDiscoverTargets() {
  std::lock_guard<std::mutex> lg(targetsMutex_);
  discoverTargets_.clear();
}

void SafeSpaceServer::onReceive(
  const sockaddr_in& peer, const uint8_t* data,
  ssize_t len, std::string& out_response) {
  // Procesamiento especial para lotes de logs enviados por CriticalEventsNode
  if (len >= 9 && std::memcmp(data, "LOG_BATCH", 9) == 0) {
    const std::string batch(reinterpret_cast<const char*>(data), static_cast<size_t>(len));
    const std::size_t newlinePos = batch.find('\n');
    const std::string logs = (newlinePos == std::string::npos) ? std::string{} : batch.substr(newlinePos + 1);

    if (logs.empty()) {
      LogManager::instance().warning("SafeSpaceServer received empty LOG_BATCH payload");
      return;
    }

    if (!storageNode.client) {
      LogManager::instance().error("SafeSpaceServer has no StorageNode client for LOG_BATCH forwarding");
      return;
    }

    std::string payload;
    payload.reserve(1 + logs.size());
    payload.push_back(static_cast<char>(MessageType::STORE_BITACORA));
    payload.append(logs);

    try {
      storageNode.client->sendRaw(payload.data(), payload.size());
      std::cout << "[SafeSpaceServer] Forwarded LOG_BATCH to StorageNode with "
                << std::count(logs.begin(), logs.end(), '\n') << " entries" << std::endl;
    } catch (const std::exception& ex) {
      LogManager::instance().error(std::string("SafeSpaceServer failed to forward LOG_BATCH: ") + ex.what());
    }
    return;
  }

  // Verificar si es un log del LogManager (empieza con "LOG")
  if (len >= 5 && data[0] == 'L' && data[1] == 'O' && data[2] == 'G') {
    // Es un log del AuthNode, reenviarlo al master
    auto& logger = LogManager::instance();

    // Parsear el nivel de log
    uint8_t level = data[3];
    uint8_t nodeNameLen = data[4];

    if (len >= 5 + nodeNameLen) {
      std::string nodeName(reinterpret_cast<const char*>(data + 5), nodeNameLen);
      std::string message(reinterpret_cast<const char*>(data + 5 + nodeNameLen), len - 5 - nodeNameLen);

      // Reenviar log al master con prefijo [FROM_<nodeName>]
      LogLevel logLevel = static_cast<LogLevel>(level);
      // Evitar reenvío recursivo si el mensaje ya contiene el prefijo
      std::string marker = "[FROM_" + nodeName + "]";
      if (message.find(marker) == std::string::npos) {
        logger.log(logLevel, "[FROM_" + nodeName + "] " + message);
        std::cout << "[SafeSpaceServer] Forwarded log from " << nodeName << " to CriticalEventsNode" << std::endl;
      } else {
        // Mensaje ya fue reenviado anteriormente; descartarlo para evitar bucle
        std::cout << "[SafeSpaceServer] Dropping recursive forwarded log from " << nodeName << std::endl;
      }
    }
    return; // No generar respuesta para logs
  }

  // DISCOVER (2 bytes)
  if (len == 2) {
    std::array<uint8_t, 2> a = { data[0], data[1] };
    DiscoverRequest d = DiscoverRequest::fromBytes(a);
    std::cout << "SafeSpaceServer: DISCOVER from msg_id=" << static_cast<int>(d.msgId()) << std::endl;

    // Remember original requester to forward future responses for this msg_id
    {
      std::lock_guard<std::mutex> lg(pendingMutex_);
      pendingRequesters_[d.msgId()] = peer;
    }

    // Forward raw bytes to all registered discover targets
    std::lock_guard<std::mutex> lg(targetsMutex_);
    for (const auto& target : discoverTargets_) {
      ssize_t sent = ::sendto(sockfd_,
                              reinterpret_cast<const void*>(data),
                              static_cast<socklen_t>(len),
                              0,
                              reinterpret_cast<const sockaddr*>(&target),
                              sizeof(target));
      if (sent < 0) {
        std::cerr << "SafeSpaceServer: forward to target failed: " << std::strerror(errno) << std::endl;
      } else {
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &target.sin_addr, ipbuf, sizeof(ipbuf));
        std::cout << "SafeSpaceServer: forwarded DISCOVER msg_id=" << static_cast<int>(d.msgId())
                  << " to " << ipbuf << ":" << ntohs(target.sin_port) << std::endl;
      }
    }

    // No immediate response to the original requester from this server; leave out_response empty.
    out_response.clear();
    return;
  }

  // DISCOVER_RESP (4 bytes)
  if (len == 4) {
    std::array<uint8_t, 4> a = { data[0], data[1], data[2], data[3] };
    DiscoverResponse resp = DiscoverResponse::fromBytes(a);
    uint8_t mid = resp.msgId();
    std::cout << "SafeSpaceServer: DISCOVER_RESP msg_id=" << static_cast<int>(mid) << std::endl;

    sockaddr_in requester{};
    bool found = false;
    {
      std::lock_guard<std::mutex> lg(pendingMutex_);
      auto it = pendingRequesters_.find(mid);
      if (it != pendingRequesters_.end()) {
        requester = it->second;
        // Optionally remove mapping (one-shot)
        pendingRequesters_.erase(it);
        found = true;
      }
    }

    if (found) {
      // forward raw response bytes back to original requester
      ssize_t sent = ::sendto(sockfd_,
                              reinterpret_cast<const void*>(data),
                              static_cast<socklen_t>(len),
                              0,
                              reinterpret_cast<const sockaddr*>(&requester),
                              sizeof(requester));
      if (sent < 0) {
        std::cerr << "SafeSpaceServer: forward response to requester failed: " << std::strerror(errno) << std::endl;
      } else {
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &requester.sin_addr, ipbuf, sizeof(ipbuf));
        std::cout << "SafeSpaceServer: forwarded DISCOVER_RESP msg_id=" << static_cast<int>(mid)
                  << " to original requester " << ipbuf << ":" << ntohs(requester.sin_port) << std::endl;
      }
    } else {
      std::cout << "SafeSpaceServer: no pending requester for msg_id=" << static_cast<int>(mid) << std::endl;
    }

    // no further response from server itself
    out_response.clear();
    return;
  }

  if (len == sizeof(GetSensorDataRequest)) {
    const auto* pkt = reinterpret_cast<const GetSensorDataRequest*>(data);
    // Obtener IP y puerto del remitente
    char ipbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

    std::cout << "[SafeSpaceServer] GetSensorDataRequest recibido desde "
              << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;

    {
      std::lock_guard<std::mutex> lg(pendingMutex_);
      // Assume GetSensorDataRequest has field sessionId (uint16_t)
      pendingRequesters_[pkt->sessionId] = peer;
    }

    try {
      storageNode.client->sendRaw(pkt, sizeof(GetSensorDataRequest));
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
    auto resp = GetSensorDataResponse::fromBytes(data, len);

    if (resp.status != 0 || resp.payload.size() > 0) { // Respuesta válida
      char ipbuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

      std::cout << "[SafeSpaceServer] GetSensorDataResponse recibido desde "
                << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;
      std::cout << "  sessionId: " << resp.sessionId << std::endl;
      std::cout << "  status: " << static_cast<int>(resp.status) << std::endl;
      std::cout << "  payload size: " << resp.payload.size() << " bytes" << std::endl;

      // Buscar cliente original para reenviar
      sockaddr_in originalClient;
      bool found = false;
      {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        auto it = pendingRequesters_.find(resp.sessionId);
        if (it != pendingRequesters_.end()) {
          originalClient = it->second;
          found = true;
          pendingRequesters_.erase(it);
        }
      }

      if (found) {
        try {
          // Reenviar al cliente original (ProxyNode)
          proxyNode.client->sendRaw(data, len);
          std::cout << "[SafeSpaceServer] Respuesta reenviada al cliente original" << std::endl;
        } catch (const std::exception& ex) {
          std::cerr << "[SafeSpaceServer] Error reenviando respuesta: "
                    << ex.what() << std::endl;
        }
      } else {
        std::cout << "[SafeSpaceServer] No se encontró cliente para sessionId="
                  << resp.sessionId << std::endl;
      }

      out_response = "ACK_GET_SENSOR_RESPONSE";
      return;
    }
  }

  if (len == sizeof(DeleteSensorDataRequest)) {
    const auto* pkt = reinterpret_cast<const DeleteSensorDataRequest*>(data);

    char ipbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

    std::cout << "[SafeSpaceServer] DeleteSensorDataRequest recibido desde "
              << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;
    std::cout << "  sessionId: " << pkt->sessionId << std::endl;
    std::cout << "  sensorId: " << pkt->sensorId << std::endl;

    try {
      storageNode.client->sendRaw(pkt, len);

      // Registrar para reenviar respuesta
      {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        pendingRequesters_[pkt->sessionId] = peer;
      }

    } catch (const std::exception& ex) {
      std::cerr << "[SafeSpaceServer] Exception al reenviar DeleteSensorDataRequest: "
                << ex.what() << std::endl;
    }

    out_response = "ACK_DELETE_SENSOR_REQUEST";
    return;
  }

  if (len == sizeof(DeleteSensorDataResponse)) {
    // Verificar si es DeleteSensorDataResponse por el MSG_ID
    if (data[0] == 0x95) { // MSG_ID de DeleteSensorDataResponse
      auto resp = DeleteSensorDataResponse::fromBytes(data, len);

      char ipbuf[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

      std::cout << "[SafeSpaceServer] DeleteSensorDataResponse recibido desde "
                << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;
      std::cout << "  sessionId: " << resp.sessionId << std::endl;
      std::cout << "  status: " << static_cast<int>(resp.status) << std::endl;

      // Buscar cliente original
      sockaddr_in originalClient;
      bool found = false;
      {
        std::lock_guard<std::mutex> lock(pendingMutex_);
        auto it = pendingRequesters_.find(resp.sessionId);
        if (it != pendingRequesters_.end()) {
          originalClient = it->second;
          found = true;
          pendingRequesters_.erase(it);
        }
      }

      if (found) {
        try {
          proxyNode.client->sendRaw(data, len);
          std::cout << "[SafeSpaceServer] Respuesta DELETE reenviada al cliente original" << std::endl;
        } catch (const std::exception& ex) {
          std::cerr << "[SafeSpaceServer] Error reenviando respuesta DELETE: "
                    << ex.what() << std::endl;
        }
      }

      out_response = "ACK_DELETE_SENSOR_RESPONSE";
      return;
    }
  }


  if (len == sizeof(SensorData)) {
    const auto* pkt = reinterpret_cast<const SensorData*>(data);

    // Obtener IP y puerto del remitente
    char ipbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

    std::cout << "[SafeSpaceServer] SENSOR_DATA recibido desde "
              << ipbuf << ":" << ntohs(peer.sin_port) << std::endl;
    std::cout << "  ▸ Temperatura: " << pkt->temperature << " °C" << std::endl;
    std::cout << "  ▸ Distancia: " << pkt->distance << " cm" << std::endl;
    std::cout << "  ▸ Presión: " << pkt->pressure<< " Pa" << std::endl;
    std::cout << "  ▸ Presión a nivel de mar: " << pkt->sealevelPressure << " cm" << std::endl;
    std::cout << "  ▸ Altitud: " << pkt->altitude << " m" << std::endl;
    std::cout << "  ▸ Altitud Real: " << pkt->realAltitude << " cm" << std::endl;

    try {
      storageNode.client->sendRaw(pkt, sizeof(SensorData));
      proxyNode.client->sendRaw(pkt, sizeof(SensorData));
    } catch (const std::exception& ex) {
      std::cerr << "[SafeSpaceServer] Exception al reenviar SENSOR_PACKET: "
                << ex.what() << std::endl;
    }

    // Generar respuesta ACK simple al emisor original (Arduino / Intermediario)
    std::string ack = "ACK_SENSOR";
    out_response.assign(ack.begin(), ack.end());
    return;
  }


  // Fallback: let base class behavior handle it (echo)
  UDPServer::onReceive(peer, data, len, out_response);
}

GetSensorDataResponse SafeSpaceServer::sendGetSensorData(uint16_t sensorId, const Token16& token) {
  GetSensorDataRequest req;
  req.sensorId = sensorId;
  req.token = token;

  std::vector<uint8_t> bytes = req.toBytes();
  this->storageNode.client->sendRaw(bytes.data(), bytes.size());

  // Receive raw response from UDP socket
  uint8_t buffer[65536];
  int fd = storageNode.client->getSocketFd();
  ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

  return GetSensorDataResponse::fromBytes(buffer, n);
}

DeleteSensorDataResponse SafeSpaceServer::sendDeleteSensorData(
  uint16_t sensorId, const Token16& token) {
  // Build request datagram
  DeleteSensorDataRequest req;
  req.sensorId = sensorId;
  req.token = token;

  std::vector<uint8_t> bytes = req.toBytes();

  // Send
  storageNode.client->sendRaw(bytes.data(), bytes.size());

  // Receive raw response from UDP socket
  uint8_t buffer[256];
  int fd = storageNode.client->getSocketFd();
  ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

  return DeleteSensorDataResponse::fromBytes(buffer, n);
}
