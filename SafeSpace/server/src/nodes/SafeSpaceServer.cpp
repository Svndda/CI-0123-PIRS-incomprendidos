#include "SafeSpaceServer.h"
#include "../model//structures/DiscoverRequest.h"
#include "../model//structures/DiscoverResponse.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

SafeSpaceServer::SafeSpaceServer(const std::string& ip, uint16_t port)
  : UDPServer(ip, port, 1024) {
  std::cout << "SafeSpaceServer: initialized on port " << port << std::endl;
}

SafeSpaceServer::~SafeSpaceServer() = default;

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

  // Fallback: let base class behavior handle it (echo)
  UDPServer::onReceive(peer, data, len, out_response);
}
