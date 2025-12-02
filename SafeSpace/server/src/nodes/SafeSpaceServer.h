#ifndef SERVER_SAFESPACESERVER_H
#define SERVER_SAFESPACESERVER_H

#include "interfaces/UDPServer.h"
#include <vector>
#include <map>
#include <mutex>
#include <string>
#include <thread>

// Critical events node
#include "CriticalEvents/CriticalEventsNode.h"
#include "interfaces/UDPClient.h"
#include <GetSensorDataResponse.h>
#include <Token.h>
#include <DeleteSensorDataResponse.h>

/**
 * @brief SafeSpaceServer routes datagrams between endpoints.
 *
 */
class SafeSpaceServer final : public UDPServer {
public:
  /** Construct and bind server on port. */
  explicit SafeSpaceServer(
    const std::string &ip, uint16_t port,
    const std::string &storageIp, uint16_t storagePort,
    const std::string &eventsIp, uint16_t eventsPort,
    const std::string &proxyIp, uint16_t proxyPort
    );

  /** Virtual destructor. */
  ~SafeSpaceServer() override;

  /** Add an endpoint (ip, port) to receive forwarded DISCOVER messages. */
  void addDiscoverTarget(const std::string& ip, uint16_t port);

  /** Remove all discover targets (simple utility). */
  void clearDiscoverTargets();

  void runInternalTests();

protected:
  /** Override onReceive to implement retransmission logic. */
  void onReceive(const sockaddr_in& peer,
                 const uint8_t* data,
                 ssize_t len,
                 std::string& out_response) override;

private:
  /** Helper: create sockaddr_in from ip/port */
  static sockaddr_in makeSockaddr(const std::string& ip, uint16_t port);

  struct StorageServerInfo{
    UDPClient* client;  //< Client to communicate with the Authentication node.
    std::string ip;     //< Authentication node ip address.
    uint16_t port;      //< Authentication node ipd address port.

    StorageServerInfo(const std::nullptr_t null, std::string  string, uint16_t uint16)
    : client(null), ip(std::move(string)), port(uint16) {};
  } storageNode;

  struct EventsServerInfo{
    UDPClient* client;  //< Client to communicate with the Authentication node.
    std::string ip;     //< Authentication node ip address.
    uint16_t port;      //< Authentication node ipd address port.

    EventsServerInfo(const std::nullptr_t null, std::string  string, uint16_t uint16)
    : client(null), ip(std::move(string)), port(uint16) {};
  } eventsNode;

  struct ProxyServerInfo{
    UDPClient* client;  //< Client to communicate with the Authentication node.
    std::string ip;     //< Authentication node ip address.
    uint16_t port;      //< Authentication node ipd address port.

    ProxyServerInfo(const std::nullptr_t null, std::string  string, uint16_t uint16)
    : client(null), ip(std::move(string)), port(uint16) {};
  } proxyNode;

  // discover forwarding targets
  std::vector<sockaddr_in> discoverTargets_{};
  std::mutex targetsMutex_;

  // map msg_id -> requester
  std::map<uint8_t, sockaddr_in> pendingRequesters_{};
  std::mutex pendingMutex_;

  // Critical events collector (runs in background)
  CriticalEventsNode* criticalEventsNode_{nullptr};
  std::thread criticalThread_;

  GetSensorDataResponse sendGetSensorData(uint16_t sensorId, const Token16& token);

  DeleteSensorDataResponse sendDeleteSensorData(
    uint16_t sensorId,
    const Token16& token);
};

#endif // SERVER_SAFESPACESERVER_H
