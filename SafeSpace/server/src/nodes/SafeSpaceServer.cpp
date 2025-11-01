#include "SafeSpaceServer.h"
#include "../model//structures/DiscoverRequest.h"
#include "../model//structures/DiscoverResponse.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

SafeSpaceServer::SafeSpaceServer(const std::string& ip, const uint16_t port,
  const std::string& storageIp, const uint16_t storagePort,
  const std::string& eventsIp, const uint16_t eventsPort,
  const std::string& proxyIp, const uint16_t proxyPort)
  : UDPServer(ip, port, 1024)
  , storageNode(nullptr, storageIp, storagePort)
  , eventsNode(nullptr, eventsIp, eventsPort)
  , proxyNode(nullptr, proxyIp, proxyPort) {
  std::cout << "SafeSpaceServer: initialized on port " << port << std::endl;
  // Start critical events node on a nearby port (port+1) to collect logs from nodes
  try {
    // uint16_t critPort = static_cast<uint16_t>(port + 1);
    // criticalEventsNode_ = new CriticalEventsNode(critPort);
    // criticalThread_ = std::thread([this]() {
    //   if (criticalEventsNode_) criticalEventsNode_->serveBlocking();
    // });
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
  } catch (const std::exception &ex) {
    std::cerr << "SafeSpaceServer: failed to start CriticalEventsNode: " << ex.what() << std::endl;
  }
}

SafeSpaceServer::~SafeSpaceServer() {
  // stop critical events node if running
  if (criticalEventsNode_) {
    criticalEventsNode_->stop();
    // UDPServer::stop signals serveBlocking to finish; join thread
    if (criticalThread_.joinable()) criticalThread_.join();
    delete criticalEventsNode_;
    criticalEventsNode_ = nullptr;
  }
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

void SafeSpaceServer::onReceive(const sockaddr_in& peer,
                                const uint8_t* data,
                                ssize_t len,
                                std::string& out_response) {
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

  // 3️⃣ Manejo de SENSOR_DATA (ArduinoNode)
  if (len == 5 && data[0] == 0x42) {
    struct SensorPacket {
      uint8_t  msgId;
      int16_t  temp_x100;
      int16_t  hum_x100;
    } __attribute__((packed));

    const SensorPacket* pkt = reinterpret_cast<const SensorPacket*>(data);

    // Convertir desde orden de red a host
    double temperature = ntohs(pkt->temp_x100) / 100.0;
    double humidity    = ntohs(pkt->hum_x100) / 100.0;

    // Obtener IP y puerto del remitente
    char ipbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));

    std::cout << "SafeSpaceServer: SENSOR_DATA recibido desde "
              << ipbuf << ":" << ntohs(peer.sin_port)
              << " -> Temp=" << temperature
              << "°C  Hum=" << humidity << "%" << std::endl;

    // Forward SensorPacket to Storage Node and Proxy Node
    try {
      // Lazy initialization: create clients only once
          // ======= ENVÍO A STORAGE NODE =======
      sockaddr_in storageAddr{};
      storageAddr.sin_family = AF_INET;
      storageAddr.sin_port = htons(storageNode.port);
      if (inet_aton(storageNode.ip.c_str(), &storageAddr.sin_addr) == 0) {
        throw std::runtime_error("[SafeSpaceServer] Invalid StorageNode IP: " + storageNode.ip);
      }

      ssize_t sentToStorage = ::sendto(
        storageNode.client->getSocketFd(),
        data,
        static_cast<size_t>(len),
        0,
        reinterpret_cast<const sockaddr*>(&storageAddr),
        sizeof(storageAddr)
      );

      if (sentToStorage < 0) {
        std::cerr << "[SafeSpaceServer] ERROR sending SENSOR_DATA to StorageNode: "
                  << std::strerror(errno) << std::endl;
      } else if (static_cast<size_t>(sentToStorage) != static_cast<size_t>(len)) {
        std::cerr << "[SafeSpaceServer] WARNING: Incomplete send to StorageNode "
                  << "(sent " << sentToStorage << " of " << len << " bytes)" << std::endl;
      } else {
        std::cout << "[SafeSpaceServer] Forwarded SENSOR_DATA to StorageNode at "
                  << storageNode.ip << ":" << storageNode.port << std::endl;
      }

      // ======= ENVÍO A PROXY NODE =======
      sockaddr_in proxyAddr{};
      proxyAddr.sin_family = AF_INET;
      proxyAddr.sin_port = htons(proxyNode.port);
      if (inet_aton(proxyNode.ip.c_str(), &proxyAddr.sin_addr) == 0) {
        throw std::runtime_error("[SafeSpaceServer] Invalid ProxyNode IP: " + proxyNode.ip);
      }

      ssize_t sentToProxy = ::sendto(
        proxyNode.client->getSocketFd(),
        data,
        static_cast<size_t>(len),
        0,
        reinterpret_cast<const sockaddr*>(&proxyAddr),
        sizeof(proxyAddr)
      );

      if (sentToProxy < 0) {
        std::cerr << "[SafeSpaceServer] ERROR sending SENSOR_DATA to ProxyNode: "
                  << std::strerror(errno) << std::endl;
      } else if (static_cast<size_t>(sentToProxy) != static_cast<size_t>(len)) {
        std::cerr << "[SafeSpaceServer] WARNING: Incomplete send to ProxyNode "
                  << "(sent " << sentToProxy << " of " << len << " bytes)" << std::endl;
      } else {
        std::cout << "[SafeSpaceServer] Forwarded SENSOR_DATA to ProxyNode at "
                  << proxyNode.ip << ":" << proxyNode.port << std::endl;
      }


    } catch (const std::exception& ex) {
      std::cerr << "[SafeSpaceServer] Exception while forwarding SENSOR_DATA: "
                << ex.what() << std::endl;
    }

    // (Opcional) Generar respuesta ACK simple
    std::string ack = "ACK_SENSOR";
    out_response.assign(ack.begin(), ack.end());
    return;
  }

  // Fallback: let base class behavior handle it (echo)
  UDPServer::onReceive(peer, data, len, out_response);
}
