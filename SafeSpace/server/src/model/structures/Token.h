#ifndef TOKEN_H
#define TOKEN_H

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

/**
 * @brief Represents a 16-byte token used for authentication/authorization.
 */
struct Token16 {
  std::array<uint8_t, 16> bytes{};

  Token16() = default;

  explicit Token16(const uint8_t* data) {
    std::memcpy(bytes.data(), data, 16);
  }

  /**
   * @brief Serializes the token into an output buffer.
   */
  void writeTo(std::vector<uint8_t>& out) const {
    out.insert(out.end(), bytes.begin(), bytes.end());
  }
};

#endif
