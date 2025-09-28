#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include "user.h"
#include "FileSystem.h"

class UserController {
private:
    std::vector<User> users;
    FileSystem* fileSystem;
    std::string userFile = "UserList";
public:
    UserController(FileSystem* fs);
    ~UserController();
    void saveUser(const User& user);
    void loadUsers();
    bool authenticate(const std::string& username, const std::string& password);
    void listUsers();
    bool deleteUser(const std::string& username);
    bool updateUser(const std::string& username, const User& updatedUser);
    User getUser(const std::string& username);
};
