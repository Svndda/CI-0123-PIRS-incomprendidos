#include "UsersManager.h"

#include <algorithm>

UsersManager::UsersManager(FileSystem& fs) : fileSystem(fs) {
  // Leer contenido actual del archivo
  this->fileSystem.openFile(this->userFile);
  
  loadUsers();
}

UsersManager::~UsersManager() {
    // Cleanup if necessary
  this->fileSystem.closeFile(this->userFile);  
}

bool UsersManager::saveUser(const User& user) {
  // Avoid duplicates
  if (std::any_of(
      users.begin(), users.end(),
      [&](const User& u) {return u.getUsername() == user.getUsername();})
    ) {
    std::cout << "User already exists.";
    return false;
  }
  
  // Append new serialized user to file
  std::string current = this->fileSystem.read(this->userFile);
  current += user.serialize();
  this->fileSystem.write(this->userFile, current);
  
  users.push_back(user);
  std::cout << "User saved successfully:" << user.getUsername();
  return true;
}


void UsersManager::loadUsers(){
  std::string data = this->fileSystem.read(this->userFile);
  if (data.empty()) {
    std::cout << "No users found.";
    return;
  }
  
  size_t start = 0;
  while (true) {
    size_t end = data.find(';', start);
    if (end == std::string::npos) break;
    
    std::string entry = data.substr(start, end - start);
    try {
      this->users.push_back(User::deserialize(entry));
    } catch (const std::exception& e) {
      std::cout << "Skipping malformed user entry:" << e.what();
    }
    
    start = end + 1;
  }
  std::cout << "Users loaded successfully:" << this->users.size();
}
    
 
bool UsersManager::authenticate(const std::string& username,
  const std::string& password) {
  auto it = std::remove_if(
      users.begin(), users.end(),
      [&](const User& u) {
        return u.getUsername() == username;
      }
  );
  
  if (it == users.end()) {
    std::cout << "User not found:" << username;
    return false;
  }
  
  bool valid = it->verifySimplePassword(password);
  if (!valid) {
    std::cout << "Invalid password for user:" << username;
  }
  
  return valid;
}

bool UsersManager::deleteUser(const std::string& username){
  auto it = std::remove_if(
      users.begin(), users.end(),
      [&](const User& u) {
      return u.getUsername() == username;
    }
  );
  
  if (it == users.end()) {
    std::cout << "User not found:" << username;
    return false;
  }
  
  users.erase(it, users.end());
  this->saveAllToFile();
  std::cout << "User deleted successfully.";
  return true;
}

bool UsersManager::updateUser(const std::string& username, const User& updatedUser){
  for (auto& u : users) {
    if (u.getUsername() == username) {
      u = updatedUser;
      this->saveAllToFile();
      std::cout << "User updated successfully.";
      return true;
    }
  }
  std::cout << "User not found:" << username;
  return false;
}
  
User UsersManager::findUser(const std::string& username){
  for (const auto& user : this->users) {
    if (user.getUsername() == username) {
      return user;
    }
  }
  std::cout << "User not found." << std::endl;
  return {"",""};
}

void UsersManager::saveAllToFile() {
  std::string data;
  for (const auto& u : users)
    data += u.serialize();
  this->fileSystem.write(this->userFile, data);
}

