//
// Created by Aaroncz on 7/10/2025.
//

#include "UDPServer.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <errno.h>
#include <utility>
#include <vector>

UDPServer::UDPServer(std::string ip, uint16_t port, size_t bufsize)
  : sockfd_(-1),
    port_(port),
    ip_(std::move(ip)),
    bufsize_(bufsize),
    handler_{},
    running_(false)
{
  // create UDP socket
  sockfd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd_ < 0) {
    throw std::runtime_error(std::string("socket() failed: ") + std::strerror(errno));
  }

  // allow of address reuse
  int opt = 1;
  if (setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    ::close(sockfd_);
    throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno));
  }

  // bind to all interfaces on the provided port
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port_);

  // Convert IP from string to binary
  if (::inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) <= 0) {
    ::close(sockfd_);
    throw std::runtime_error(std::string("Invalid IP address: ") + ip_);
  }

  // Bind socket to given IP and port
  if (::bind(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    ::close(sockfd_);
    throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno));
  }

  std::cout << "UDPServer: bound to " << ip_ << ":" << port_ << std::endl;
}

UDPServer::~UDPServer() {
  stop();
  if (sockfd_ >= 0) {
    ::close(sockfd_);
    sockfd_ = -1;
  }
}

void UDPServer::setHandler(Handler h) {
  handler_ = std::move(h);
}

void UDPServer::serveBlocking() {
  running_.store(true);
  std::vector<uint8_t> buffer(bufsize_);

  std::cout << "UDPServer: entering blocking serve loop..." << std::endl;

  while (running_.load()) {
    sockaddr_in peer{};
    socklen_t peerlen = sizeof(peer);

    ssize_t received = ::recvfrom(sockfd_, buffer.data(),
      static_cast<socklen_t>(buffer.size()), 0,
      reinterpret_cast<sockaddr*>(&peer), &peerlen
      );

    if (received < 0) {
      if (errno == EINTR) {
        // interrupted by signal; check running_ flag and continue or exit
        continue;
      }
      std::cerr << "recvfrom() error: " << std::strerror(errno) << std::endl;
      continue;
    }

    // log short info
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipstr, sizeof(ipstr));
    uint16_t peerPort = ntohs(peer.sin_port);
    std::cout << "UDPServer: received " << received << " bytes from " << ipstr << ":" << peerPort << std::endl;

    // No special-case payloads here. Unblock for shutdown is handled by
    // `stop()` which sends a harmless wake packet to the bound address.

    std::string response;
    // call virtual hook
    try {
      onReceive(peer, buffer.data(), received, response);
    } catch (const std::exception& ex) {
      std::cerr << "Exception in onReceive(): " << ex.what() << std::endl;
      // Dump a small hex-preview of the received buffer to help debugging
      std::cerr << "  Received " << received << " bytes; preview: ";
      size_t preview = std::min<size_t>(static_cast<size_t>(received), 64);
      for (size_t i = 0; i < preview; ++i) {
        char b = static_cast<char>(buffer[i]);
        unsigned int ub = static_cast<unsigned char>(b);
        std::cerr << std::hex << ((ub >> 4) & 0xF) << (ub & 0xF);
        if (i + 1 < preview) std::cerr << " ";
      }
      std::cerr << std::dec << std::endl;
      continue;
    }

    // send response if present
    if (!response.empty()) {
      ssize_t sent = ::sendto(sockfd_, response.data(),
        static_cast<socklen_t>(response.size()), 0,
        reinterpret_cast<sockaddr*>(&peer), peerlen
        );
      if (sent < 0) {
        std::cerr << "sendto() error: " << std::strerror(errno) << std::endl;
      } else {
        std::cout << "UDPServer: sent " << sent << " bytes to " << ipstr << ":" << peerPort << std::endl;
      }
    }
  }

  running_.store(false);
  std::cout << "UDPServer: leaving serve loop." << std::endl;
}

void UDPServer::stop() noexcept {
  running_.store(false);
  // Send a single-byte wake packet to our own bound address so a blocking
  // recvfrom() will return and notice `running_ == false`.
  if (sockfd_ >= 0) {
    try {
      int s = ::socket(AF_INET, SOCK_DGRAM, 0);
      if (s >= 0) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        if (::inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) > 0) {
          const uint8_t wake = 0x00;
          ::sendto(s, &wake, 1, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        ::close(s);
      }
    } catch (...) {
      // best-effort; never throw from stop()
    }
  }
}

void UDPServer::sendTo(const sockaddr_in& peer, const uint8_t* data, size_t len) const {
  if (sockfd_ < 0) {
    throw std::runtime_error("socket closed");
  }

  ssize_t sent = ::sendto(sockfd_, reinterpret_cast<const void*>(data),
    static_cast<socklen_t>(len), 0,
    reinterpret_cast<const sockaddr*>(&peer), sizeof(peer)
    );
  if (sent < 0 || static_cast<size_t>(sent) != len) {
    throw std::runtime_error(std::string("sendto failed: ") + std::strerror(errno));
  }

}

void UDPServer::onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len, std::string& out_response) {
  // Default implementation
  if (handler_) {
    handler_(peer, data, len, out_response);
    return;
  }

  // Default echo behavior
  // out_response.assign(reinterpret_cast<const char*>(data), static_cast<size_t>(len));
}