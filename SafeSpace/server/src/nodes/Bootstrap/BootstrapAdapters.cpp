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
#include "SafeSpaceServer.h"

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
      try { st->node->start(); } catch (const std::exception &e) {
        std::cerr << "[StorageAdapter] Exception in storage start thread: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[StorageAdapter] Unknown exception in storage start thread" << std::endl;
      }
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

// Master (SafeSpaceServer) adapter
std::pair<StartCb, StopCb> makeMasterAdapter(
  const std::string& bindIp, uint16_t bindPort,
  const std::string& storageIp, uint16_t storagePort,
  const std::string& eventsIp, uint16_t eventsPort,
  const std::string& proxyIp, uint16_t proxyPort) {
  struct State { SafeSpaceServer* server = nullptr; std::thread t; std::mutex m; };
  auto st = std::make_shared<State>();

  StartCb start = [st, bindIp, bindPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->server) return true;
    try {
      st->server = new SafeSpaceServer(bindIp, bindPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort);
    } catch (const std::exception &e) {
      std::cerr << "[MasterAdapter] Failed to construct SafeSpaceServer: " << e.what() << std::endl;
      return false;
    }
    st->t = std::thread([st]() {
      try { st->server->serveBlocking(); } catch (const std::exception &e) {
        std::cerr << "[MasterAdapter] serveBlocking exception: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[MasterAdapter] serveBlocking unknown exception" << std::endl;
      }
      std::lock_guard<std::mutex> lg(st->m);
      delete st->server; st->server = nullptr;
    });
    return true;
  };

  StopCb stop = [st]() -> bool {
    SafeSpaceServer* s = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      s = st->server;
    }
    if (!s) return true;
    try { s->stop(); } catch (...) { }
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
      try { n->stop(); } catch (...) { }
    if (st->t.joinable()) st->t.join();
    return true;
  };

  return {start, stop};
}

// Intermediary adapter
std::pair<StartCb, StopCb> makeIntermediaryAdapter(
  int listenPort, const std::string& masterIp, int masterPort) {
  struct State { IntermediaryNode* node = nullptr; std::mutex m; };
  auto st = std::make_shared<State>();

  // For IntermediaryNode::start() the node creates its own worker thread and
  // returns immediately. Therefore the adapter must NOT spawn a separate
  // thread that would delete the node after start() returns. Instead we
  // allocate the node and call start() synchronously; stop() will request
  // shutdown and we delete the node in the stop callback.
  StartCb start = [st, listenPort, masterIp, masterPort]() -> bool {
    std::lock_guard<std::mutex> lg(st->m);
    if (st->node) return true;
    st->node = new IntermediaryNode(listenPort, masterIp, masterPort);
    try {
      bool ok = st->node->start();
      if (!ok) {
        delete st->node; st->node = nullptr;
        return false;
      }
    } catch (...) {
      delete st->node; st->node = nullptr;
      return false;
    }
    return true;
  };

  StopCb stop = [st]() -> bool {
    IntermediaryNode* n = nullptr;
    {
      std::lock_guard<std::mutex> lg(st->m);
      n = st->node;
      st->node = nullptr; // mark as gone for other callers
    }
    if (!n) return true;
    try { n->stop(); } catch (...) { }
    delete n;
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
