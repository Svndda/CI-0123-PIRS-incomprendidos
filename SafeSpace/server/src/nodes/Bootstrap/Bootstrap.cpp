#include "Bootstrap.h"
#include <iostream>
#include <stdexcept>

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
  uint8_t status = 1;  // 1 = SUCCESS; could be dynamically computed

  std::cout << "[Bootstrap] RUN request → node " << int(nodeId)
            << " status OK" << std::endl;

  // Response format: MSG_ID | NODE_ID | STATUS
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
  uint8_t status = 1;  // Same success flag

  std::cout << "[Bootstrap] STOP request → node " << int(nodeId)
            << " stopped successfully" << std::endl;

  return StopNodeResponse(nodeId, status);
}
