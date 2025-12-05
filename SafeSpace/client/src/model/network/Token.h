#ifndef TOKEN_H
#define TOKEN_H

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <stdexcept>
#include <iomanip>
#include <sstream>

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
     * @brief Creates a Token16 from a string.
     * @param str A string of exactly 16 characters (binary data)
     * @throws std::invalid_argument if string length is not 16
     */
  explicit Token16(const std::string& str) {
    if (str.size() != 16) {
      throw std::invalid_argument("Token string must be exactly 16 bytes");
    }
    std::memcpy(bytes.data(), str.data(), 16);
  }
  
  /**
     * @brief Creates a Token16 from a hex string.
     * @param hexStr A hex string of exactly 32 characters (16 bytes in hex)
     * @throws std::invalid_argument if hex string is invalid
     */
  static Token16 fromHexString(const std::string& hexStr) {
    if (hexStr.size() != 32) {
      throw std::invalid_argument("Hex string must be exactly 32 characters");
    }
    
    Token16 token;
    for (size_t i = 0; i < 16; ++i) {
      std::string byteStr = hexStr.substr(i * 2, 2);
      token.bytes[i] = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
    }
    return token;
  }
  
  /**
     * @brief Converts token to a hex string representation.
     */
  std::string toHexString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : bytes) {
      ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
  }
  
  /**
     * @brief Converts token to a binary string (16 bytes).
     */
  std::string toString() const {
    return std::string(reinterpret_cast<const char*>(bytes.data()), 16);
  }
  
  /**
     * @brief Serializes the token into an output buffer.
     */
  void writeTo(std::vector<uint8_t>& out) const {
    out.insert(out.end(), bytes.begin(), bytes.end());
  }
  
  /**
     * @brief Compares two tokens for equality.
     */
  bool operator==(const Token16& other) const {
    return bytes == other.bytes;
  }
  
  /**
     * @brief Compares two tokens for inequality.
     */
  bool operator!=(const Token16& other) const {
    return !(*this == other);
  }
  
  /**
     * @brief Checks if token is empty (all zeros).
     */
  bool isEmpty() const {
    for (const auto& byte : bytes) {
      if (byte != 0) return false;
    }
    return true;
  }
};

#endif // TOKEN_H
