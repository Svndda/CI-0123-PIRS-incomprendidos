#ifndef PROXYNODE_H
#define PROXYNODE_H

#include "../interfaces/UDPServer.h"
#include "../interfaces/UDPClient.h"
#include <mutex>
#include <unordered_map>
#include <arpa/inet.h>
#include <thread>
#include <utility>

#include "SensorPacket.h"

/**
 * @brief Constructs a ProxyNode responsible for forwarding messages between clients,
 *        the authentication server, and the master (SafeSpaceServer).
 *
 * @param ip             Local IP address to bind.
 * @param proxyPort      Listening port for the ProxyNode.
 * @param authServerIp   IP address of the authentication server.
 * @param authServerPort Port of the authentication server.
 * @param masterServerIp IP address of the master (SafeSpaceServer).
 * @param masterServerPort Port of the master server.
 */
class ProxyNode : public UDPServer {
private:
  /**
   * @struct AuthServerInfo
   * @brief Holds information required for communication with the authentication server.
   */
  struct AuthServerInfo {
    UDPClient *client; //< Client to communicate with the Authentication node.
    std::string ip; //< Authentication node ip address.
    uint16_t port; //< Authentication node ipd address port.

    AuthServerInfo(const std::nullptr_t null, std::string string, uint16_t uint16)
      : client(null), ip(std::move(string)), port(uint16) {
    };
  } authNode;

  /**
   * @struct MasterServerInfo
   * @brief Holds information required for communication with the master SafeSpaceServer.
   */
  struct MasterServerInfo {
    UDPClient *client; //< Client to communicate with the Master node.
    std::string ip; //< Master node ip address.
    uint16_t port; //< Master node ipd address port.
    MasterServerInfo(const std::nullptr_t null, std::string string, const uint16_t uint16)
      : client(null), ip(std::move(string)), port(uint16) {
    };
  } masterNode;

  /**
   * @struct ClientInfo
   * @brief Holds metadata about a connected client.
   */
  struct ClientInfo {
    sockaddr_in addr;
    uint8_t msgId;
  };

  std::atomic<bool> listening;               ///< Flag controlling listener thread state.
  std::thread listenerThread;                ///< Thread handling incoming auth server responses.

  std::mutex clientsMutex;                   ///< Synchronization for pending clients map.
  std::unordered_map<uint16_t, ClientInfo> pendingClients; ///< Map of sessionId to client info.

  std::mutex authenticatedMutex;                   ///< Synchronization for authenticated clients map.
  std::unordered_map<uint16_t, ClientInfo> authenticatedClients; ///< Map of sessionId to client info.

  std::mutex subscribersMutex;               ///< Synchronization for subscriber list.
  std::unordered_map<uint16_t, ClientInfo> subscribers; ///< Active subscribers for sensor data broadcasting.

  std::mutex failedAttemptsMutex;            ///< Protects failed login attempt map.
  std::unordered_map<std::string, int> failedAttempts; ///< Tracks failed login attempts by IP.

public:
  /**
   * @brief Constructs a ProxyNode instance.
   * @param ip Local IP address for the proxy node.
   * @param proxyPort UDP port on which this proxy will listen.
   * @param authServerIp IP address of the authentication server.
   * @param authServerPort Port number of the authentication server.
   * @param masterServerIp IP address of the master SafeSpaceServer.
   * @param masterServerPort Port number of the master SafeSpaceServer.
   */
  ProxyNode(const std::string &ip, uint16_t proxyPort, const std::string &authServerIp,
            uint16_t authServerPort, const std::string &masterServerIp, uint16_t masterServerPort);

  /**
   * @brief Destructor. Ensures clean shutdown and resource deallocation.
   */
  ~ProxyNode() override;

  /**
   * @brief Starts the proxy node operation.
   *
   * Launches the listener thread for authentication server responses and
   * begins the blocking UDP server loop.
   */
  void start();

protected:
  /**
   * @brief Main dispatcher for incoming UDP datagrams.
   * @param peer Sender socket address.
   * @param data Raw datagram data.
   * @param len Length of the datagram.
   * @param out_response Output string for optional immediate response.
   */
  void onReceive(const sockaddr_in &peer, const uint8_t *data, ssize_t len,
                 std::string &out_response) override;

private:
  /**
   * @brief Handles a connection subscription request from a client.
   * @param peer Sender address.
   * @param data Raw packet data.
   * @param len Packet length.
   * @param out_response Response string (e.g., "SUBSCRIBED_OK").
   */
  void handleConnectRequest(const sockaddr_in &peer, const uint8_t *data,
                            ssize_t len, std::string &out_response);

  /**
   * @brief Handles incoming authentication requests and forwards them to AuthNode.
   * @param peer Sender address.
   * @param data Raw packet data.
   * @param len Packet length.
   * @param out_response Output response (empty, as forwarding is asynchronous).
   */
  void handleAuthRequest(const sockaddr_in &peer, const uint8_t *data,
                         ssize_t len, std::string &out_response);

  /**
   * @brief Handles sensor data packets and broadcasts to all subscribers.
   * @param peer Sender address.
   * @param data Raw sensor packet.
   * @param len Packet length.
   * @param out_response ACK response to sender.
   */
  void handleSensorData(const sockaddr_in &peer, const uint8_t *data,
                        ssize_t len, std::string &out_response);

  /**
   * @brief Handles sensor history queries; validates session and forwards to master.
   */
  void handleSensorQuery(const sockaddr_in &peer, const uint8_t *data,
                         ssize_t len, std::string &out_response);

  /**
   * @brief Handles log messages received from AuthNode and forwards to master.
   * @param peer Sender address (unused).
   * @param data Raw log packet data.
   * @param len Packet length.
   */
  void handleLogMessage(const sockaddr_in &peer, const uint8_t *data, ssize_t len);

  /**
   * @brief Fallback handler for unknown message formats.
   * @param peer Sender address.
   * @param data Raw packet data.
   * @param len Packet length.
   * @param out_response Default echo or warning message.
   */
  void handleUnknownMessage(const sockaddr_in &peer, const uint8_t *data,
                            ssize_t len, std::string &out_response);

  /**
   * @brief Forwards a datagram to the authentication server.
   * @param data Pointer to data buffer.
   * @param len Size of data buffer.
   */
  void forwardToAuthServer(const uint8_t *data, size_t len);

  /**
   * @brief Forwards a datagram to the master server.
   * @param data Pointer to data buffer.
   * @param len Size of data buffer.
   */
  void forwardToMasterServer(const uint8_t *data, size_t len);

  /**
   * @brief Listens continuously for authentication server responses.
   *
   * Runs in a dedicated thread, dispatching valid responses for processing.
   */
  void listenAuthServerResponses();

  /**
   * @brief Receives data from the authentication server socket.
   * @param buffer Destination buffer.
   * @param authAddr Output address of the sender.
   * @param addrLen Size of the sockaddr_in structure.
   * @return ssize_t Number of bytes received, or -1 on timeout/error.
   */
  ssize_t receiveFromAuthServer(std::vector<uint8_t>& buffer, sockaddr_in& authAddr, socklen_t& addrLen);

  /**
   * @brief Processes and routes incoming messages from the authentication server.
   * @param buffer Message buffer.
   * @param length Buffer length.
   * @param authAddr Address of the authentication server.
   */
  void processAuthServerResponse(const std::vector<uint8_t>& buffer, size_t length, const sockaddr_in& authAddr);

  /**
   * @brief Handles and forwards DISCOVER_RESP messages to their original clients.
   * @param buffer 4-byte DiscoverResponse message.
   */
  void handleDiscoverResponse(const std::vector<uint8_t>& buffer);

  /**
   * @brief Handles and forwards AUTH_RESPONSE messages back to the requesting client.
   * @param buffer Raw AuthResponse buffer.
   * @param length Length of the AuthResponse buffer.
   */
  void handleAuthResponse(const uint8_t *buffer, size_t length);

  void handleGetSystemUsersResponse(const uint8_t *buffer, size_t length);

  /**
   * @brief Registers a new broadcast subscriber client.
   * @param addr Client socket address.
   * @param sessionId Unique session identifier.
   */
  void registerSubscriber(const sockaddr_in &addr, uint16_t sessionId);

  /**
   * @brief Stores client session information for pending responses.
   * @param sessionId Session identifier.
   * @param addr Client address.
   */
  void storePendingClient(uint16_t sessionId, const sockaddr_in &addr);

  /**
   * @brief Removes a pending client from the session map.
   * @param sessionId Session identifier to remove.
   */
  void removePendingClient(uint16_t sessionId);

  /**
   * @brief Broadcasts raw binary data to all registered subscribers.
   * @param data Pointer to data buffer.
   * @param len Size of the data buffer.
   */
  void broadcastToSubscribers(const uint8_t* data, size_t len);

  /**
   * @brief Converts a sockaddr_in structure to a readable string "IP:Port".
   * @param addr Socket address.
   * @return std::string IP:Port formatted string.
   */
  std::string sockaddrToString(const sockaddr_in &addr) const;

  /**
   * @brief Registers a client as authenticated.
   * @param addr Client address.
   * @param sessionId Client session identifier.
   */
  void registerAuthenticatedClient(const sockaddr_in &addr, uint16_t sessionId);

  /**
   * @brief Checks if a client is already authenticated.
   * @param sessionId Session identifier to check.
   * @return true if authenticated, false otherwise.
   */
  bool isClientAuthenticated(uint16_t sessionId);
};

#endif // PROXYNODE_H
