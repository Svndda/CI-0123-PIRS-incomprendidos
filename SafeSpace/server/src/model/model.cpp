// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include <vector>
#include <limits>

#include "FileSystem.h"
#include "model.h"

Model::Model()
  : filesystem(UNITY_PATH), usersManager(UsersManager(filesystem)) {
  // USUARIOS HARDCODEADOS
  User u1("realAdmin", "Administrador del sistema");
  u1.setPassword("M2sv8KxpLq");
  this->usersManager.saveUser(u1);
  
  User u2("dataAdmin", "Administrador de datos");
  u2.setPassword("N7vbq2R0");
  this->usersManager.saveUser(u2);
  
  User u3("audiTT", "Auditor");
  u3.setPassword("gH5pxL9pQ");
  this->usersManager.saveUser(u3);
  
  User u4("guestAA", "Invitado 1");
  u4.setPassword("aB7nvZt9Ow1");
  this->usersManager.saveUser(u4);
  
  User u5("guestBB", "Invitado 2");
  u5.setPassword("z9dsRk5Tg");
  this->usersManager.saveUser(u5);
}

Model& Model::getInstance() {
  // Creates an static instance of the POS MODEL.
  static Model instance;
  return instance;
}

bool Model::start(const User& user) {
  // Returns the model state flag.
  return this->started;
}

bool Model::authenticate(
    const std::string& username, const std::string& password) {
  return this->usersManager.authenticate(username, password);
}

bool Model::deleteUser(
    const std::string& username) {
  return this->usersManager.deleteUser(username);
}

bool Model::deleteUser(
    const User& user) {
  return this->usersManager.deleteUser(user.getUsername());
}

bool Model::updateUser(
    const std::string& username, const User& updatedUser) {
  return this->usersManager.updateUser(username, updatedUser);
}

bool Model::saveUser(
    const std::string &username, const std::string &password, const std::string &rol) {
  User newUser(username, rol);
  newUser.setPassword(password);
  return this->usersManager.saveUser(newUser);
}
