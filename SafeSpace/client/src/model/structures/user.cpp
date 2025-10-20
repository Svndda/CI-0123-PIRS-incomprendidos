#include "user.h"
#include <QCryptographicHash>

User::User(std::string _username, std::string _passwordHash,
    std::string _group, uint16_t _permissions,
    uint16_t _failedAttemps, bool _isLocked)
    : username(std::move(_username)), passwordHash(std::move(_passwordHash)),
    group(std::move(_group)), permissions(_permissions),
    failedAttemps(_failedAttemps), isLocked(_isLocked) {
}

bool User::operator==(const User& other) const {
  return (this->username == other.username &&
          this->passwordHash == other.passwordHash &&
          this->group == other.group &&
          this->permissions == other.permissions &&
          this->failedAttemps == other.failedAttemps &&
          this->isLocked == other.isLocked);
}

bool User::operator!=(const User& other) const {
  return !(*this == other);
}

User& User::operator=(const User& other) {
  // Avoid reasignation.
  if (this != &other) {
    // Avoids autoassignation.
    this->username = other.username;
    this->passwordHash = other.passwordHash;
    this->group = other.group;
    this->permissions = other.permissions;
    this->failedAttemps = other.failedAttemps;
    this->isLocked = other.isLocked;
  }
  
  // Returns the actual object.
  return *this;
}

const std::string& User::getUsername() const {
  return this->username;
}

void User::setUsername(const std::string name) {
  this->username = name;
}

void User::setPassword(const std::string& newHash) {
  this->passwordHash = hashSHA256(newHash);
}

bool User::verifyPassword(const std::string& passwordToCheck) const {
  std::string hashAttempt = hashSHA256(passwordToCheck);
  return this->passwordHash == hashAttempt;
}
