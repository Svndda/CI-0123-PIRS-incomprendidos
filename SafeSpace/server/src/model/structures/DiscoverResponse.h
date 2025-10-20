//
// Created by Aaroncz on 7/10/2025.
//

#ifndef SERVER_DISCOVERRESPONSE_H
#define SERVER_DISCOVERRESPONSE_H
#include <array>
#include <cstdint>
#include <stdexcept>

/*
  * @brief Binary representation for a DISCOVER_RESP datagram.
  *
  * DISCOVER_RESP format (4 bytes):
  *  - MSG_ID    : 1 byte
  *  - SENSOR_ID : 1 byte
  *  - TYPE      : 1 byte
  *  - STATUS    : 1 byte
  *
  * This header provides a POD-like struct with helpers for
  * serialization and a stream operator for logging/debugging.
  */
class DiscoverResponse {

  uint8_t msg_id;     ///< Message identifier (1 byte)
  uint8_t sensor_id;  ///< Sensor identifier (1 byte)
  uint8_t rtype;       ///< Sensor type (1 byte)
  uint8_t rstatus;     ///< Status flag (1 byte)

public:

  /**
   * @brief Default / value constructor.
   * @param msg_id message id
   * @param sensor_id sensor identifier
   * @param type sensor type
   * @param status status byte
   */
  explicit DiscoverResponse(const uint8_t msg_id = 0,
                       const uint8_t sensor_id = 0,
                       const uint8_t type = 0,
                       const uint8_t status = 0)
    : msg_id(msg_id), sensor_id(sensor_id), rtype(type), rstatus(status) {}

  /** @brief Copy constructor (defaulted). */
  DiscoverResponse(const DiscoverResponse&) = default;

  /** @brief Move constructor (defaulted). */
  DiscoverResponse(DiscoverResponse&&) noexcept = default;

  /**
   * @brief Serialize to a 4-byte array.
   */
  std::array<uint8_t, 4> toBytes() const noexcept {
    return { this->msg_id, this->sensor_id, this->rtype, this->rstatus};
  }

  /**
   * @brief Construct from a 4-byte array.
   */
  static DiscoverResponse fromBytes(const std::array<uint8_t, 4>& a) {
    return DiscoverResponse{ a[0], a[1], a[2], a[3] };
  }

  /**
   * @brief Construct from a raw buffer.
   * @throws std::invalid_argument if len != 4
   */
  static DiscoverResponse fromBytes(const uint8_t* data, size_t len) {
    if (len != 4) {
      throw std::invalid_argument("DiscoverResponse::fromBytes: invalid length");
    }
    return DiscoverResponse{ data[0], data[1], data[2], data[3] };
  }

public: ///> Getters.

  /** @brief Accessors */
  uint8_t msgId() const noexcept { return this->msg_id; }
  uint8_t sensorId() const noexcept { return this->sensor_id; }
  uint8_t type() const noexcept { return this->rtype; }
  uint8_t status() const noexcept { return this->rstatus; }

  void setMsgId(uint8_t v) noexcept { this->msg_id = v; }
  void setSensorId(uint8_t v) noexcept { this->sensor_id = v; }
  void setType(uint8_t v) noexcept { this->rtype = v; }
  void setStatus(uint8_t v) noexcept { this->rstatus = v; }

public: ///> Operators
  /**
   * @brief Copy assignment.
   * @return reference to *this
   */
  DiscoverResponse& operator=(const DiscoverResponse& other) {
    if (this != &other) {
      msg_id = other.msg_id;
      sensor_id = other.sensor_id;
      rtype = other.rtype;
      rstatus = other.rstatus;
    }
    return *this;
  }

  /** @brief Equality operator. */
  bool operator==(const DiscoverResponse& other) const noexcept {
    return msg_id == other.msg_id &&
           sensor_id == other.sensor_id &&
           rtype == other.rtype &&
           rstatus == other.rstatus;
  }

  /** @brief Inequality operator. */
  bool operator!=(const DiscoverResponse& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Stream insertion operator for human-readable logging.
   * @param os output stream
   * @param d datagram to print
   * @return reference to the output stream
   */
  friend std::ostream& operator<<(std::ostream& os, const DiscoverResponse& d) {
    os << "DISCOVER_RESP MSG_ID=" << static_cast<int>(d.msg_id)
       << " SENSOR_ID=" << static_cast<int>(d.sensor_id)
       << " TYPE=" << static_cast<int>(d.rtype)
       << " STATUS=" << static_cast<int>(d.rstatus);
    return os;
  }
};
#endif //SERVER_DISCOVERRESPONSE_H