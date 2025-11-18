#include "Bootstrap.h"
#include <iostream>
#include <stdexcept>

#include <utility>

/**
 * @brief Bootstrap constructor.
 */
Bootstrap::Bootstrap(const std::string& ip,
                   uint16_t port,
                   size_t bufsize)
  : UDPServer(ip, port, bufsize) {
  std::cout << "[Bootstrap] Boot server initialized on "
            << ip << ":" << port << std::endl;
}

void Bootstrap::registerNode(uint8_t nodeId, StartCallback startCb, StopCallback stopCb) {
  std::lock_guard<std::mutex> lg(registryMutex_);
  registry_[nodeId] = NodeHandler{startCb, stopCb, false};
  std::cout << "[Bootstrap] Registered node handler for id=" << int(nodeId) << std::endl;
}

void Bootstrap::unregisterNode(uint8_t nodeId) {
  std::lock_guard<std::mutex> lg(registryMutex_);
  registry_.erase(nodeId);
  std::cout << "[Bootstrap] Unregistered node handler for id=" << int(nodeId) << std::endl;
}

/**
 * @brief Central handler for all received datagrams.
 *
 * This method inspects datagram size:
 *  - 2 bytes → RUN_NODE_REQUEST or STOP_NODE_REQUEST
 *  - otherwise ignored.
 */
void Bootstrap::onReceive(const sockaddr_in& peer,
                         const uint8_t* data,
                         ssize_t len,
                         std::string& out_response) {
  if (len == 2) {
    const uint8_t msgId = data[0];

    // RUN_NODE_REQUEST
    if (msgId == 0x7b) {
      RunNodeResponse resp = handleRunNodeRequest(data, len);

      // Serialize to 3 bytes
      auto arr = resp.toBytes();
      out_response.assign(reinterpret_cast<const char*>(arr.data()),
                          arr.size());
      return;
    }

    // STOP_NODE_REQUEST
    if (msgId == 0x7d) {
      StopNodeResponse resp = handleStopNodeRequest(data, len);

      auto arr = resp.toBytes();
      out_response.assign(reinterpret_cast<const char*>(arr.data()),
                          arr.size());
      return;
    }
  }

  std::cout << "[Bootstrap] Received unknown datagram (" << len
            << " bytes), ignoring." << std::endl;
}

/**
 * @brief Handles a RUN_NODE_REQUEST and produces a response.
 */
RunNodeResponse Bootstrap::handleRunNodeRequest(const uint8_t* data,
                                               ssize_t len)
{
  if (len != 2) {
    throw std::invalid_argument("Invalid RUN_NODE_REQUEST length");
  }

  RunNodeRequest req = RunNodeRequest::fromBytes(data, len);

  uint8_t nodeId = req.nodeId();
  uint8_t status = 0; // default: failure

  // Lookup handler
  NodeHandler handlerCopy;
  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it == registry_.end()) {
      std::cout << "[Bootstrap] RUN request → node " << int(nodeId)
                << " not registered" << std::endl;
      return RunNodeResponse(0x7c, nodeId, status);
    }
    // if already running, respond OK (idempotent)
    if (it->second.running) {
      std::cout << "[Bootstrap] RUN request → node " << int(nodeId)
                << " already running" << std::endl;
      return RunNodeResponse(0x7c, nodeId, 1);
    }
    handlerCopy = it->second;
  }

  bool started = false;
  if (handlerCopy.start) {
    try {
      started = handlerCopy.start();
    } catch (const std::exception &ex) {
      std::cerr << "[Bootstrap] Exception while starting node " << int(nodeId) << ": " << ex.what() << std::endl;
      started = false;
    } catch (...) {
      std::cerr << "[Bootstrap] Unknown exception while starting node " << int(nodeId) << std::endl;
      started = false;
    }
  }

  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it != registry_.end()) it->second.running = started;
  }

  status = started ? 1 : 0;
  std::cout << "[Bootstrap] RUN request → node " << int(nodeId)
            << " status " << int(status) << std::endl;

  return RunNodeResponse(0x7c, nodeId, status);
}

/**
 * @brief Handles a STOP_NODE_REQUEST and produces a response.
 */
StopNodeResponse Bootstrap::handleStopNodeRequest(const uint8_t* data,
                                                 ssize_t len)
{
  if (len != 2) {
    throw std::invalid_argument("Invalid STOP_NODE_REQUEST length");
  }

  StopNodeRequest req = StopNodeRequest::fromBytes(data, len);

  uint8_t nodeId = req.nodeId();
  uint8_t status = 0;

  NodeHandler handlerCopy;
  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it == registry_.end()) {
      std::cout << "[Bootstrap] STOP request → node " << int(nodeId)
                << " not registered" << std::endl;
      return StopNodeResponse(nodeId, status);
    }
    if (!it->second.running) {
      std::cout << "[Bootstrap] STOP request → node " << int(nodeId)
                << " already stopped" << std::endl;
      return StopNodeResponse(nodeId, 1);
    }
    handlerCopy = it->second;
  }

  bool stopped = false;
  if (handlerCopy.stop) {
    try {
      stopped = handlerCopy.stop();
    } catch (const std::exception &ex) {
      std::cerr << "[Bootstrap] Exception while stopping node " << int(nodeId) << ": " << ex.what() << std::endl;
      stopped = false;
    } catch (...) {
      std::cerr << "[Bootstrap] Unknown exception while stopping node " << int(nodeId) << std::endl;
      stopped = false;
    }
  }

  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it != registry_.end()) it->second.running = !stopped ? it->second.running : false;
  }

  status = stopped ? 1 : 0;
  std::cout << "[Bootstrap] STOP request → node " << int(nodeId)
            << " status " << int(status) << std::endl;

  return StopNodeResponse(nodeId, status);
}

bool Bootstrap::startNode(uint8_t nodeId) {
  NodeHandler handler;
  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it == registry_.end()) return false;
    if (it->second.running) return true;
    handler = it->second;
  }

  bool started = false;
  if (handler.start) {
    try {
      started = handler.start();
    } catch (...) {
      started = false;
    }
  }

  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it != registry_.end()) it->second.running = started;
  }
  return started;
}

bool Bootstrap::stopNode(uint8_t nodeId) {
  NodeHandler handler;
  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it == registry_.end()) return false;
    if (!it->second.running) return true;
    handler = it->second;
  }

  bool stopped = false;
  if (handler.stop) {
    try {
      stopped = handler.stop();
    } catch (...) {
      stopped = false;
    }
  }

  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    auto it = registry_.find(nodeId);
    if (it != registry_.end() && stopped) it->second.running = false;
  }
  return stopped;
}

std::vector<std::pair<uint8_t, bool>> Bootstrap::listNodes() const {
  std::vector<std::pair<uint8_t, bool>> out;
  std::lock_guard<std::mutex> lg(registryMutex_);
  for (const auto& kv : registry_) {
    out.emplace_back(kv.first, kv.second.running);
  }
  return out;
}

void Bootstrap::stopAllNodes() {
  std::vector<uint8_t> ids;
  {
    std::lock_guard<std::mutex> lg(registryMutex_);
    for (const auto& kv : registry_) ids.push_back(kv.first);
  }
  for (auto id : ids) {
    try { stopNode(id); } catch (...) { }
  }
}
