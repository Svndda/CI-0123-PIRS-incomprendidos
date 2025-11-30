
#ifndef STOP_NODE_RESPONSE_H
#define STOP_NODE_RESPONSE_H

#include <cstdint>
#include <array>
#include <ostream>

/**
 * @brief Binary representation for a STOP_NODE_RESPONSE datagram.
 */

class StopNodeResponse {
private:
  static constexpr uint8_t msg_id = 0x7e;     ///< Message identifier (1 byte)
  uint8_t node_id;   ///< Node identifier  (1 byte)
  uint8_t rstatus;     ///< Stop Response status (1 byte)

public:
  /**
   * @brief Default / value constructor.
   * @param msg_id message identifier
   * @param node_id Node identifier
   * @param rstatus  flags byte
   */
  explicit StopNodeResponse(uint8_t nodeId = 0, uint8_t status = 0)
    : node_id(nodeId), rstatus(status) {
  }

  /** @brief Copy constructor (defaulted). */
  StopNodeResponse(const StopNodeResponse&) = default;

  /** @brief Move constructor (defaulted). */
  StopNodeResponse(StopNodeResponse&&) noexcept = default;

  /**
 * @brief Construct from a raw buffer.
 * @param data pointer to bytes (must not be null if len > 0)
 * @param len number of bytes available
 * @throws std::invalid_argument if len != 3
 */
  static StopNodeResponse fromBytes(const uint8_t* data, size_t len) {
    if (len != 3) {
      throw std::invalid_argument("StopNodeResponse::fromBytes: invalid length");
    }
    return StopNodeResponse{ data[1], data[2] };
  }

  /**
   * @brief Serialize to a 3-byte array.
   */
  std::array<uint8_t, 3> toBytes() const noexcept {
    return { msg_id, node_id, rstatus };
  }

  /**
   * @brief Construct from a 3-byte array.
   * @throws std::invalid_argument if length is not 2 (not applicable here).
   */
  static StopNodeResponse fromBytes(const std::array<uint8_t, 3>& a) {
    return StopNodeResponse{ a[1], a[2]};
  }

  /**
 * @brief Copy assignment.
 * @return reference to *this
 */
  StopNodeResponse& operator=(const StopNodeResponse& other) {
    if (this != &other) {
      this->node_id = other.node_id;
      this->rstatus = other.rstatus;
    }
    return *this;
  }

  /** @brief Equality operator. */
  bool operator==(const StopNodeResponse& other) const noexcept {
    return this->node_id == other.node_id && this->rstatus == other.rstatus;
  }

  /** @brief Inequality operator. */
  bool operator!=(const StopNodeResponse& other) const noexcept {
    return !(*this == other);
  }

public:
  /** @brief Accessors */
  uint8_t nodeId() const noexcept { return this->node_id; }
  uint8_t status() const noexcept { return this->rstatus; }

  void setNodeId(uint8_t v) noexcept { this->node_id = v; }
  void setStatus(uint8_t v) noexcept { this->rstatus = v; }
  
    /**
 * @brief Stream insertion operator for human-readable logging.
 * @param os output stream
 * @param d datagram to print
 * @return reference to the output stream
 */
  friend std::ostream& operator<<(std::ostream& os, const StopNodeResponse& d) {
    os << "[STOP_NODE_RESPONSE] MSG_ID=" << static_cast<int>(d.msg_id)
       << " NODE_ID=" << static_cast<int>(d.node_id)
       << " STATUS=" << static_cast<int>(d.rstatus);
    return os;
  }
};

#endif //STOP_NODE_RESPONSE_H
