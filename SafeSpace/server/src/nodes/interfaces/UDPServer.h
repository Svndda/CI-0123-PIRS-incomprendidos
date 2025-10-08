//
// Created by Aaroncz on 7/10/2025.
//

#ifndef SERVER_UDPSERVER_H
#define SERVER_UDPSERVER_H

#include <cstdint>
#include <functional>
#include <string>
#include <atomic>
#include <netinet/in.h>

/**
 * @brief Simple blocking UDP server.
 *
 * By default, the server listen for incoming datagrams. The user
 * can provide a custom handler callback to implement application logic.
 *
 * Threading model:
 *  - All public methods are callable from any thread, but serveBlocking() runs
 *    in the caller thread and is blocking until stop() is called or a special
 *    shutdown payload (leading '#') is received.
 */
class UDPServer {

public:
  /**
   * @brief Handler type invoked for each received datagram.
   *
   * @brief Receive handler type.
   * @param peer sender address (sockaddr_in).
   * @param data pointer to the received bytes.
   * @param len length of received bytes.
   * @param out_response response string to send back if non-empty.
   */
  using Handler = std::function<void(const sockaddr_in& peer,
                                     const uint8_t* data,
                                     ssize_t len,
                                     std::string& out_response)>;

 protected:
  int sockfd_;
  uint16_t port_;
  size_t bufsize_;
  Handler handler_;
  std::atomic<bool> running_;

 public:
  /**
   * @brief Construct and bind a UDP socket to the given port.
   * @param port host-order port number to bind (e.g. 5000).
   * @param bufsize receive buffer size (bytes) used internally.
   * @throws std::runtime_error on socket or bind failure.
   */
  explicit UDPServer(uint16_t port, size_t bufsize = 1024);

  /**
   * @brief Destructor. Closes socket and releases resources.
   */
  virtual ~UDPServer();

  /**
 * @brief Hook invoked on each received datagram.
 *
 * Default implementation:
 *  - If a handler_ callback is set, calls it.
 *  - Otherwise, echoes the payload back.
 *
 * Derived classes should override this method to implement protocol logic.
 *
 * @param peer sender address
 * @param data received bytes
 * @param len number of bytes received
 * @param out_response string to populate with response bytes (if any)
 */
  virtual void onReceive(const sockaddr_in& peer,
                         const uint8_t* data,
                         ssize_t len,
                         std::string& out_response);

  /**
   * @brief Set an optional receive handler.
   * If no handler is set the server will echo received payloads.
   */
  void setHandler(Handler h);


  /**
   * @brief Blocking serve loop. Caller thread will block on recvfrom().
   * The loop exits when stop() is called or when a shutdown payload ('#') is received.
   */
  void serveBlocking();

  /**
   * @brief Request a graceful stop from another thread.
   */
  void stop() noexcept;

  /**
   * @brief Send raw bytes to a specific peer (throws on error).
   */
  void sendTo(const sockaddr_in& peer, const uint8_t* data, size_t len) const;

  /**
   * @brief Returns bound port (host order).
   */
  uint16_t port() const noexcept { return port_; }

 private:
  // Non-copyable
  UDPServer(const UDPServer&) = delete;
  UDPServer& operator=(const UDPServer&) = delete;
};

#endif //SERVER_UDPSERVER_H