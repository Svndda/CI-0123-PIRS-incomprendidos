#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include "user.h"
#include "Raid1FileSystem.h"

class UsersManager {
private:
  std::vector<User> users;
  Raid1FileSystem& fileSystem;
  std::string userFile = "UserList";
public:
  UsersManager(Raid1FileSystem& fs);
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
private:
  void saveAllToFile();
};
