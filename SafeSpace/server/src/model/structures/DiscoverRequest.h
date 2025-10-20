//
// Created by Aaroncz on 7/10/2025.
//

#ifndef SERVER_DISCOVERREQUEST_H
#define SERVER_DISCOVERREQUEST_H

#include <cstdint>
#include <array>
#include <ostream>

/**
 * @brief Binary representation for a DISCOVER datagram.
 */

class DiscoverRequest {
private:
  uint8_t msg_id;  ///< Message identifier (1 byte)
  uint8_t r_flags;   ///< Flags / discovery type (1 byte)

public:
  /**
   * @brief Default / value constructor.
   * @param msg_id message identifier
   * @param flags  flags byte
   */
  explicit DiscoverRequest(const uint8_t msg_id = 0, uint8_t flags = 0)
    : msg_id(msg_id), r_flags(flags) {
  }

  /** @brief Copy constructor (defaulted). */
  DiscoverRequest(const DiscoverRequest&) = default;

  /** @brief Move constructor (defaulted). */
  DiscoverRequest(DiscoverRequest&&) noexcept = default;

  /**
 * @brief Construct from a raw buffer.
 * @param data pointer to bytes (must not be null if len > 0)
 * @param len number of bytes available
 * @throws std::invalid_argument if len != 2
 */
  static DiscoverRequest fromBytes(const uint8_t* data, size_t len) {
    if (len != 2) {
      throw std::invalid_argument("DiscoverRequest::fromBytes: invalid length");
    }
    return DiscoverRequest{ data[0], data[1] };
  }

  /**
   * @brief Serialize to a 2-byte array.
   */
  std::array<uint8_t, 2> toBytes() const noexcept {
    return { msg_id, r_flags };
  }

  /**
   * @brief Construct from a 2-byte array.
   * @throws std::invalid_argument if length is not 2 (not applicable here).
   */
  static DiscoverRequest fromBytes(const std::array<uint8_t, 2>& a) {
    return DiscoverRequest{ a[0], a[1] };
  }

  /**
 * @brief Copy assignment.
 * @return reference to *this
 */
  DiscoverRequest& operator=(const DiscoverRequest& other) {
    if (this != &other) {
      msg_id = other.msg_id;
      this->r_flags = other.r_flags;
    }
    return *this;
  }

  /** @brief Equality operator. */
  bool operator==(const DiscoverRequest& other) const noexcept {
    return msg_id == other.msg_id && this->r_flags == other.r_flags;
  }

  /** @brief Inequality operator. */
  bool operator!=(const DiscoverRequest& other) const noexcept {
    return !(*this == other);
  }

public:
  /** @brief Accessors */
  uint8_t msgId() const noexcept { return msg_id; }
  uint8_t rflags() const noexcept { return r_flags; }

  void setMsgId(uint8_t v) noexcept { msg_id = v; }
  void setr_flags(uint8_t v) noexcept { r_flags = v; }

  /**
 * @brief Stream insertion operator for human-readable logging.
 * @param os output stream
 * @param d datagram to print
 * @return reference to the output stream
 */
  friend std::ostream& operator<<(std::ostream& os, const DiscoverRequest& d) {
    os << "[DISCOVER] MSG_ID=" << static_cast<int>(d.msg_id)
       << " this->drflags=" << static_cast<int>(d.r_flags);
    return os;
  }

};

#endif //SERVER_DISCOVERREQUEST_H