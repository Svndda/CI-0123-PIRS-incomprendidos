#ifndef USER_H
#define USER_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <iomanip>

/**
 * @class User
 * @brief Represents a user in the program system.
 *
 * The User class stores a user's unique identifier, name, hashed password,
 * and a set of page access permissions. It provides functionality to set and
 * verify the password as well as serialize/deserialize.
 */
class User {
private:
  std::string username;            ///< Name of the user.
  std::string passwordHash;            ///< Hashed password.
  std::string group;
  uint16_t permissions;
  uint16_t failedAttemps;
  bool locked;
public:
  
  /**
   * @brief Constructs a User object.
   *
   * Initializes the User with a given id, name, and an optional set of page access permissions.
   *
   */
  User(std::string _username = "", std::string _passwordHash = "",
       std::string _group = "Usuario" , uint16_t _permissions = 4,
       uint16_t _failedAttemps = 0, bool _isLocked = false
       );
  
  /**
   * @brief Destructor.
   */
  ~User() = default;
public:
  
  static std::string hashSHA256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(
               input.c_str()), input.size(), hash
           );

    // Convert hash bytes to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hash[i]);
    }

    // Convert to std::string
    std::string hashHex = ss.str();

    // SHA-256 normally produces 64 chars (32 bytes)
    if (hashHex.size() > 32) {
      hashHex = hashHex.substr(0, 32);
    }

    return hashHex;
  }
  
  static User deserialize(const std::string& line) {
    size_t firstComma = line.find(',');
    size_t secondComma = line.find(',', firstComma + 1);
    
    if (firstComma == std::string::npos || secondComma == std::string::npos)
      throw std::runtime_error("Invalid user serialization format");
    
    std::string group = line.substr(0, firstComma);
    std::string username = line.substr(firstComma + 1, secondComma - firstComma - 1);
    std::string passwordHash = line.substr(secondComma + 1);
    
    return User(username, passwordHash, group);
  }
  
  /**
   * @brief Equality operator.
   *
   * Compares two User objects based on their username, hashed password, group and permissions.
   *
   * @param other The other User object to compare.
   * @return True if the users attributes match; otherwise, false.
   */
  bool operator==(const User& other) const;
  
  /**
   * @brief Unequality operator.
   *
   * Compares two User objects based on their username, hashed password, group and permissions.
   *
   * @param other The other User object to compare.
   * @return True if the users attributes does not match; otherwise, false.
   */
  bool operator!=(const User& other) const;
  
  /**
   * @brief Assignment operator.
   *
   * Copies the attributes from another User into this instance.
   *
   * @param other The User object to copy from.
   * @return Reference to the current object.
   */
  User& operator=(const User& other);
  
  /**
   * @brief Retrieves the user's username.
   *
   * @return Constant reference to the user's username string.
   */
  const std::string& getUsername() const;
  
  std::string serialize() const {
    return group + "," + username + "," + passwordHash + ";";
  }  
  
  /**
   * @brief Sets the user's name.
   *
   * @param name The new name to set.
   */
  void setUsername(const std::string& name);
  
  /**
   * @brief Sets the user's password.
   *
   * Hashes the provided password string and stores it.
   *
   * @param password The new password to hash and set.
   */
  void setPassword(const std::string& password);
  
  /**
   * @brief Verifies the provided password against the stored hash.
   *
   * @param password The password to verify.
   * @return True if the provided password matches the stored hash; otherwise, false.
   */
  bool verifySimplePassword(const std::string& password);

  /**
   * @brief Verifies the provided hashed password against the stored hash.
   *
   * @param hash The hashed password to verify.
   * @return True if the provided hashed password matches the stored hash; otherwise, false.
   */
  bool verifyHashPassword(const std::string& hash);
  
  /**
   * @brief Get the user's group.
   * 
   * @return std::string 
   */
  std::string getGroup() const {return this->group;}

  /**
   * @brief Get the user's group.
   *
   * @return std::string
   */
  bool isLocked() const {return this->locked;}
  
  /**
   * @brief Get the user's password hash.
   * 
   * @return std::string
   */
  const std::string& getPasswordHash() const { return passwordHash; }

  /**
   * @brief Sets the user's password hash.
   *
   * @param newPermissions The password to verify.
   */
   void setPermissions(const uint16_t newPermissions) { this->permissions = newPermissions; }

  /**
   * @brief Get the user's password hash.
   *
   * @param newHashedPassword The password to verify.
   */
  void setPasswordHash(const std::string& newHashedPassword) { this->passwordHash = newHashedPassword; }
  
  /**
   * @brief Sets the user's group.
   * 
   * @param newGroup
   */
  void setGroup(const std::string& newGroup) {this->group = newGroup;}
};

#endif // USER_H
