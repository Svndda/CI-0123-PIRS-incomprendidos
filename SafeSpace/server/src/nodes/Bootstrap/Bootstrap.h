#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H

#include "RunNode.h"
#include "RunNodeResponse.h"
#include "StopNode.h"
#include "StopNodeResponse.h"
#include "interfaces/UDPServer.h"
#include <functional>
#include <unordered_map>
#include <mutex>

/**
 * @class Bootstrap
 * @brief Specialized UDP server responsible for handling node-bootstrapping
 *        commands (RUN_NODE_REQUEST, STOP_NODE_REQUEST) and returning the
 *        appropriate response datagrams.
 *
 * The class listens for incoming datagrams, decodes their binary format,
 * executes the requested operation, and sends a RUN_NODE_RESPONSE or
 * STOP_NODE_RESPONSE back to the originating peer.
 *
 * The server uses the protocol definitions found in:
 *  - RunNodeRequest (2 bytes)
 *  - StopNodeRequest (2 bytes)
 *  - RunNodeResponse (3 bytes)
 *  - StopNodeResponse (3 bytes)
 *
 * The Bootstrap server runs in blocking mode through UDPServer::serveBlocking().
 */
class Bootstrap : public UDPServer {
public:

  using StartCallback = std::function<bool()>; ///< return true on success
  using StopCallback = std::function<bool()>;  ///< return true on success


  /**
   * @brief Constructs and binds a Bootstrap to given IPv4 and port.
   *
   * @param ip IPv4 string address (e.g., "127.0.0.1")
   * @param port UDP port to bind the server
   * @param bufsize Optional receive buffer size (defaults to 1024)
   *
   * @throws std::runtime_error if socket creation or binding fails.
   */
  explicit Bootstrap(const std::string& ip,
                    uint16_t port,
                    size_t bufsize = 1024);

  /**
   * @brief Virtual destructor.
   */
  ~Bootstrap() override = default;
  /**
   * @brief Register a node handler for a given node id.
   *
   * The Bootstrap will call `startCb` when a RUN_NODE_REQUEST for `nodeId`
   * arrives, and `stopCb` when a STOP_NODE_REQUEST arrives. Both callbacks
   * should be safe to call multiple times (Bootstrap will track running state).
   */
  void registerNode(uint8_t nodeId, StartCallback startCb, StopCallback stopCb);

  /**
   * @brief Unregister a previously registered node.
   */
  void unregisterNode(uint8_t nodeId);

  /**
   * @brief Start a registered node by id. Returns true on success.
   */
  bool startNode(uint8_t nodeId);

  /**
   * @brief Stop a registered node by id. Returns true on success.
   */
  bool stopNode(uint8_t nodeId);

  /**
   * @brief Return list of registered node ids and running state.
   */
  std::vector<std::pair<uint8_t, bool>> listNodes() const;

  /**
   * @brief Stop all registered nodes (best-effort).
   */
  void stopAllNodes();

protected:
  /**
   * @brief Overridden handler from UDPServer.
   *
   * Called automatically every time a datagram arrives. This method inspects
   * the datagram size, decides whether it represents a RUN or STOP request,
   * executes the appropriate operation, and generates a binary response.
   *
   * @param peer The sender socket address
   * @param data Received binary buffer
   * @param len Number of received bytes
   * @param out_response String to populate with the binary response
   */
  void onReceive(const sockaddr_in& peer,
                 const uint8_t* data,
                 ssize_t len,
                 std::string& out_response) override;


private:

  /**
   * @brief Processes a RUN_NODE_REQUEST datagram.
   *
   * @param data Pointer to raw bytes
   * @param len Number of bytes (should be exactly 2)
   * @return A RunNodeResponse object to be sent back
   */
  RunNodeResponse handleRunNodeRequest(const uint8_t* data, ssize_t len);

  /**
   * @brief Processes a STOP_NODE_REQUEST datagram.
   *
   * @param data Pointer to raw bytes
   * @param len Number of bytes (should be exactly 2)
   * @return A StopNodeResponse object to be sent back
   */
  StopNodeResponse handleStopNodeRequest(const uint8_t* data, ssize_t len);

private:

  struct NodeHandler {
    StartCallback start;
    StopCallback stop;
    bool running{false};
  };

  std::unordered_map<uint8_t, NodeHandler> registry_{};
  mutable std::mutex registryMutex_{};

};

#endif // BOOTSTRAP_H
