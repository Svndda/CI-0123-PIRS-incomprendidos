#include "UsersManager.h"

#include <algorithm>

UsersManager::UsersManager(FileSystem& fs) : fileSystem(fs),
    raid1Manager(fs, this->userFile, "UserList_mirror") {
  // Leer contenido actual del archivo
  this->fileSystem.openFile(this->userFile);

  std::cout << "Initializing UsersManager with RAID 1" << std::endl;
  
  loadUsers();
  checkRAIDStatus();
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
  std::string current = raid1Manager.read();
  current += user.serialize();
  bool success = raid1Manager.write(current);
  
  if (success) {
    users.push_back(user);
    std::cout << "User saved successfully:" << user.getUsername();
  } else {
    std::cerr << "Error: Failed to save user with RAID 1" << std::endl;
  }

  return success;
}


void UsersManager::loadUsers(){
  try {
    std::string data = raid1Manager.read();
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

  } catch (const std::exception& e) {
    std::cerr << "Error loading users: " << e.what() << std::endl;
  }
    
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

void UsersManager::checkRAIDStatus() {
    DiskStatus primaryStatus = raid1Manager.getPrimaryStatus();
    DiskStatus mirrorStatus = raid1Manager.getMirrorStatus();
    
    std::cout << "\n=== RAID 1 Status (Users) ===" << std::endl;
    
    if (primaryStatus == DiskStatus::OPERATIONAL) {
        std::cout << "Primary disk (UserList): OPERATIONAL" << std::endl;
    } else {
        std::cout << "Primary disk (UserList): FAILED" << std::endl;
    }
    
    if (mirrorStatus == DiskStatus::OPERATIONAL) {
        std::cout << "Mirror disk (UserList_mirror): OPERATIONAL" << std::endl;
    } else {
        std::cout << "Mirror disk (UserList_mirror): FAILED" << std::endl;
    }
    
    std::cout << "Active disc:: " << raid1Manager.getActiveDisk() << std::endl;
    
    if (primaryStatus == DiskStatus::FAILED || mirrorStatus == DiskStatus::FAILED) {
        std::cout << "WARNING: RAID 1 (USERS) in DEGRADED mode" << std::endl;
    } else {
        std::cout << "RAID 1 (USERS) functioning correctly" << std::endl;
    }
    
    std::cout << "==================================\n" << std::endl;
}

bool UsersManager::rebuildRAIDIfNeeded() {
    DiskStatus primaryStatus = raid1Manager.getPrimaryStatus();
    DiskStatus mirrorStatus = raid1Manager.getMirrorStatus();
    
    if (primaryStatus == DiskStatus::FAILED && mirrorStatus == DiskStatus::OPERATIONAL) {
        std::cout << "(USERS) Rebuilding primary disk from mirror..." << std::endl;
        return raid1Manager.rebuildPrimary();
    }
    
    if (mirrorStatus == DiskStatus::FAILED && primaryStatus == DiskStatus::OPERATIONAL) {
        std::cout << "(USERS) Rebuilding mirror disk from primary..." << std::endl;
        return raid1Manager.rebuildMirror();
    }
    
    std::cout << "RAID 1 (USERS) does not require rebuilding." << std::endl;
    return true;
}
