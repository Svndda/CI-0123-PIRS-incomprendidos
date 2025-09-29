#include "user.h"
#include <QCryptographicHash>
User::User(const size_t userId, const std::string userName
    , const std::vector<PageAccess> userPermissions)
    : id(userId)
    , name(userName)
    , permissions(userPermissions) {
}

User::PageAccess::PageAccess(const size_t pageIndexed, const size_t pageAccess)
    : pageIndex(pageIndexed)
    , access(pageAccess) {
}

const size_t& User::getID() const {
  // Returns the user's id.
  return this->id;
}

void User::setUserID(const size_t id) {
  // Sets the user's id.
  this->id = id;
}

const std::string& User::getUsername() const {
  // Returns the user's name.
  return this->name;
}

void User::setUsername(const std::string name) {
  // Sets the user's name.
  this->name = name;
}

void User::setPassword(const std::string& newPassword) {
  ByteArray hash = QCryptographicHash::hash(
    QByteArray::fromStdString(newPassword),
    QCryptographicHash::Sha256
  );
  this->passwordHash = QString(hash.toHex()).toStdString();
}

bool User::verifyPassword(const std::string& passwordToCheck) const {
  QByteArray hash = QCryptographicHash::hash(
    QByteArray::fromStdString(passwordToCheck),
    QCryptographicHash::Sha256
  );
  std::string hashHex = QString(hash.toHex()).toStdString();
  return this->passwordHash == hashHex;
}

const std::vector<User::PageAccess>& User::getUserPermissions() const {
  // Returns the user's permissions.
  return this->permissions;
}

void User::setUserPermissions(const std::vector<User::PageAccess> permissions) {
  // Sets the user's permissions.
  this->permissions = permissions;
}

bool User::operator==(const User& other) const {
  // Checks if the users's attributes matches.
  return (this->name == other.name)
      && (this->password == other.password);
}

bool User::operator!=(const User& other) const {
  // Checks if the users's attributes does not matches.
  return !(*this == other);
}

User& User::operator=(const User& other) {
  // Avoid reasignation.
  if (this == &other) {
    // Avoids autoassignation.
    return *this;
  }
  
  // Copy the other object attributes.  
  this->id = other.id;
  this->name = other.name;
  this->permissions = other.permissions;
  this->password = other.password;
  
  // Returns the actual object.
  return *this;
}

bool User::PageAccess::operator==(const PageAccess& other) const {
  // Checks if the page's accesses aattributes matches.
  return (this->pageIndex == other.pageIndex)
      && (this->access == other.access);;  
}

User::PageAccess& User::PageAccess::operator=(const PageAccess& other) {
  // Avoid reasignation.
  if (*this == other) {
    return *this;
  }
  // Copy the other object attributes.    
  this->pageIndex = other.pageIndex;
  this->access = other.access;
  // Returns the current object.
  return *this;
}

std::string User::getPasswordHash() const {
  return this->passwordHash;
}


