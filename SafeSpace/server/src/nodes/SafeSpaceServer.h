#ifndef SERVER_SAFESPACESERVER_H
#define SERVER_SAFESPACESERVER_H

#include "interfaces/UDPServer.h"
#include <vector>
#include <map>
#include <mutex>
#include <string>

/**
 * @brief SafeSpaceServer routes datagrams between endpoints.
 *
 */
class SafeSpaceServer final : public UDPServer {
public:
  /** Construct and bind server on port. */
  explicit SafeSpaceServer(const std::string& ip, uint16_t port);

  /** Virtual destructor. */
  ~SafeSpaceServer() override;

  /** Add an endpoint (ip, port) to receive forwarded DISCOVER messages. */
  void addDiscoverTarget(const std::string& ip, uint16_t port);

  /** Remove all discover targets (simple utility). */
  void clearDiscoverTargets();

protected:
  /** Override onReceive to implement retransmission logic. */
  void onReceive(const sockaddr_in& peer,
                 const uint8_t* data,
                 ssize_t len,
                 std::string& out_response) override;

private:
  /** Helper: create sockaddr_in from ip/port */
  static sockaddr_in makeSockaddr(const std::string& ip, uint16_t port);

  // discover forwarding targets
  std::vector<sockaddr_in> discoverTargets_{};
  std::mutex targetsMutex_;

  // map msg_id -> requester
  std::map<uint8_t, sockaddr_in> pendingRequesters_{};
  std::mutex pendingMutex_;
};

#endif // SERVER_SAFESPACESERVER_H
