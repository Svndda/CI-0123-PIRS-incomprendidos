#ifndef RUN_NODE_REQUEST_H
#define RUN_NODE_REQUEST_H

#include <cstdint>
#include <array>
#include <ostream>

/**
 * @brief Binary representation for a RUN_NODE_REQUEST datagram.
 */

class RunNodeRequest {
private:
  uint8_t msg_id;  ///< Message identifier (1 byte)
  uint8_t node_id;   ///< Node identifier  (1 byte)

public:
  /**
   * @brief Default / value constructor.
   * @param msg_id message identifier
   * @param node_id  Node identifier
   */
  explicit RunNodeRequest(const uint8_t msg_id = 0, uint8_t nodeId = 0)
    : msg_id(msg_id), node_id(nodeId) {
  }

  /** @brief Copy constructor (defaulted). */
  RunNodeRequest(const RunNodeRequest&) = default;

  /** @brief Move constructor (defaulted). */
  RunNodeRequest(RunNodeRequest&&) noexcept = default;

  /**
 * @brief Construct from a raw buffer.
 * @param data pointer to bytes (must not be null if len > 0)
 * @param len number of bytes available
 * @throws std::invalid_argument if len != 2
 */
  static RunNodeRequest fromBytes(const uint8_t* data, size_t len) {
    if (len != 2) {
      throw std::invalid_argument("RunNodeRequest::fromBytes: invalid length");
    }
    return RunNodeRequest{ data[0], data[1] };
  }

  /**
   * @brief Serialize to a 2-byte array.
   */
  std::array<uint8_t, 2> toBytes() const noexcept {
    return { this->msg_id, this->node_id };
  }

  /**
   * @brief Construct from a 2-byte array.
   * @throws std::invalid_argument if length is not 2 (not applicable here).
   */
  static RunNodeRequest fromBytes(const std::array<uint8_t, 2>& a) {
    return RunNodeRequest{ a[0], a[1] };
  }

  /**
 * @brief Copy assignment.
 * @return reference to *this
 */
  RunNodeRequest& operator=(const RunNodeRequest& other) {
    if (this != &other) {
      this->msg_id = other.msg_id;
      this->node_id = other.node_id;
    }
    return *this;
  }

  /** @brief Equality operator. */
  bool operator==(const RunNodeRequest& other) const noexcept {
    return this->msg_id == other.msg_id && this->node_id == other.node_id;
  }

  /** @brief Inequality operator. */
  bool operator!=(const RunNodeRequest& other) const noexcept {
    return !(*this == other);
  }

public:
  /** @brief Accessors */
  uint8_t msgId() const noexcept { return this->msg_id; }
  uint8_t nodeId() const noexcept { return this->node_id; }

  void setMsgId(uint8_t v) noexcept { this->msg_id = v; }
  void setNodeId(uint8_t v) noexcept { this->node_id = v; }

  /**
 * @brief Stream insertion operator for human-readable logging.
 * @param os output stream
 * @param d datagram to print
 * @return reference to the output stream
 */
  friend std::ostream& operator<<(std::ostream& os, const RunNodeRequest& d) {
    os << "[RUN_NODE_REQUEST] MSG_ID=" << static_cast<int>(d.msg_id)
       << " NODE_ID=" << static_cast<int>(d.node_id);
    return os;
  }

};

#endif //RUN_NODE_REQUEST_H