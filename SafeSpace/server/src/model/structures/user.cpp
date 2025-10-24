#include "user.h"

User::User(std::string _username, std::string _passwordHash,
    std::string _group, uint16_t _permissions,
    uint16_t _failedAttemps, bool _isLocked)
    : username(std::move(_username)), passwordHash(std::move(_passwordHash)),
    group(std::move(_group)), permissions(_permissions),
    failedAttemps(_failedAttemps), locked(_isLocked) {
}

bool User::operator==(const User& other) const {
  return (this->username == other.username &&
          this->passwordHash == other.passwordHash &&
          this->group == other.group &&
          this->permissions == other.permissions &&
          this->failedAttemps == other.failedAttemps &&
          this->locked == other.locked);
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
    this->locked = other.locked;
  }
  
  // Returns the actual object.
  return *this;
}

const std::string& User::getUsername() const {
  return this->username;
}

void User::setUsername(const std::string& name) {
  this->username = name;
}

void User::setPassword(const std::string& newHash) {
  this->passwordHash = hashSHA256(newHash);
}

bool User::verifySimplePassword(const std::string& passsword) {
  std::string hashAttempt = hashSHA256(passsword);
  if (this->passwordHash == hashAttempt) {
    return true;
  }

  this->failedAttemps++;
  if (this->failedAttemps >= 3) {
    this->locked = true;
  }

  return false;
}

bool User::verifyHashPassword(const std::string& passsword) {
  if (this->passwordHash == passsword) {
    return true;
  }

  this->failedAttemps++;
  if (this->failedAttemps >= 3) {
    this->locked = true;
  }

  return false;
}
