#include "BootstrapAdapters.h"
#include <memory>
#include <thread>
#include <mutex>
#include <iostream>


#include "Proxy/ProxyNode.h"
#include "Storage/StorageNode.h"
#include "Intermediary/IntermediaryNode.h"
#include "Auth/auth_udp_server.h"
#include "Arduino/Arduino_Node.h"

using StartCb = std::function<bool()>;
using StopCb = std::function<bool()>;

// Explicit adapter for ProxyNode
std::pair<StartCb, StopCb> makeProxyAdapter(
  const std::string& listenIp, uint16_t listenPort,
  const std::string& authIp, uint16_t authPort,
  const std::string& masterIp, uint16_t masterPort) {
  struct State { ProxyNode* node = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, listenIp, listenPort, authIp, authPort, masterIp, masterPort]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new ProxyNode(listenIp, listenPort, authIp, authPort, masterIp, masterPort);
    st->t = std::thread([st]() {
      try { st->node->start(); } catch (...) { }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->node; st->node = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    ProxyNode* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
    }
    if (!n) return true;
    try { n->stop(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}

// StorageNode adapter
std::pair<StartCb, StopCb> makeStorageAdapter(
  uint16_t storagePort, const std::string& masterIp, uint16_t masterPort,
  const std::string& nodeId, const std::string& diskPath) {
  struct State { StorageNode* node = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, storagePort, masterIp, masterPort, nodeId, diskPath]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new StorageNode(storagePort, masterIp, masterPort, nodeId, diskPath);
    st->t = std::thread([st]() {
      try { st->node->start(); } catch (...) { }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->node; st->node = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    StorageNode* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
    }
    if (!n) return true;
    try { n->~StorageNode(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}

// Intermediary adapter
std::pair<StartCb, StopCb> makeIntermediaryAdapter(
  int listenPort, const std::string& masterIp, int masterPort) {
  struct State { IntermediaryNode* node = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, listenPort, masterIp, masterPort]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new IntermediaryNode(listenPort, masterIp, masterPort);
    st->t = std::thread([st]() {
      try { st->node->start(); } catch (...) { }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->node; st->node = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    IntermediaryNode* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
    }
    if (!n) return true;
    try { n->stop(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}

// Auth adapter (UDPServer-based)
std::pair<StartCb, StopCb> makeAuthAdapter(
  const std::string& listenIp, uint16_t listenPort) {
  struct State { AuthUDPServer* node = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, listenIp, listenPort]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new AuthUDPServer(listenIp, listenPort);
    st->t = std::thread([st]() {
      try { st->node->serveBlocking(); } catch (...) { }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->node; st->node = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    AuthUDPServer* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
    }
    if (!n) return true;
    try { n->stop(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}

// Arduino adapter
std::pair<StartCb, StopCb> makeArduinoAdapter(
  const std::string& masterIp, int masterPort, const std::string& serialPath, const std::string& format) {
  struct State { ArduinoNode* node = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, masterIp, masterPort, serialPath, format]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new ArduinoNode(masterIp, masterPort, serialPath, format);
    st->t = std::thread([st]() {
      try { st->node->run(); } catch (...) { }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->node; st->node = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    ArduinoNode* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
    }
    if (!n) return true;
    try { n->stop(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}
