#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include "user.h"
#include "FileSystem.h"
#include "RAID1Manager.h"

class UsersManager {
private:
  std::vector<User> users;
  FileSystem& fileSystem;
  RAID1Manager raid1Manager;
  std::string userFile = "UserList";
public:
  UsersManager(FileSystem& fs);
  ~UsersManager();
  
public: ///> Getters
  std::vector<User> getUsers() {return this->users;}
  
public:
  
  bool saveUser(const User& user);
  void loadUsers();
  bool authenticate(const std::string& username, const std::string& password);
  void listUsers();
  bool deleteUser(const std::string& username);
  bool updateUser(const std::string& username, const User& updatedUser);
  User findUser(const std::string& username);

  void checkRAIDStatus();
  bool rebuildRAIDIfNeeded();
private:
  void saveAllToFile();
};
