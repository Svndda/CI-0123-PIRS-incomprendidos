// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include <vector>
#include <limits>

#include <QDebug>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include "FileSystem.h"
#include "model.h"

Model::Model()
  : client("127.0.0.1", 8090, this) {
  // // USUARIOS HARDCODEADOS
  // User u1("realAdmin", "Administrador del sistema");
  // u1.setPassword("M2sv8KxpLq");
  // this->usersManager.saveUser(u1);
  
  // User u2("dataAdmin", "Administrador de datos");
  // u2.setPassword("N7vbq2R0");
  // this->usersManager.saveUser(u2);
  
  // User u3("audiTT", "Auditor");
  // u3.setPassword("gH5pxL9pQ");
  // this->usersManager.saveUser(u3);
  
  // User u4("guestAA", "Invitado 1");
  // u4.setPassword("aB7nvZt9Ow1");
  // this->usersManager.saveUser(u4);
  
  // User u5("guestBB", "Invitado 2");
  // u5.setPassword("z9dsRk5Tg");
  // this->usersManager.saveUser(u5);

  std::time_t now = std::time(nullptr);

  for (int i = 0; i < 20; ++i) {
      this->sensorsData.emplace_back(
          i + 1,                // id
          now + i * 60,         // timestamp (cada uno 1 min después)
          100 + i * 5,          // distance
          i % 2,                // movement (0 o 1)
          20 + i,               // temperature
          3 + i % 5,            // uv
          40 + i * 2,           // microphone
          i % 2,                // led (on/off)
          (i % 3 == 0) ? 1 : 0, // buzzer (solo algunos activos)
          200 + i * 10          // ligth
          );
  }
}

Model& Model::getInstance() {
  // Creates an static instance of the POS MODEL.
  static Model instance;
  return instance;
}

bool Model::start(/*const User& user*/) {
  // Returns the model state flag.
  return this->started;
}

bool Model::authenticate(
    const std::string& username, const std::string& password) {
  // return this->usersManager.authenticate(username, password);
    std::cout << "Autentiando usuarios" << std::endl ;
  qDebug() << "Autenticando usuario";
  this->connect(
    &this->client, &QtUDPClient::authResponseReceived,
    this, [this](const AuthResponse& resp) {
      qInfo() << "Authentication response received:";
      qInfo() << "  Session ID:" << resp.getSessionId();
      qInfo() << "  Status:" << resp.getStatusCode();
      qInfo() << "  Message:" << QString::fromStdString(resp.getMessage());
      qInfo() << "  Token:" << QString::fromStdString(resp.getSessionToken());
      
      if (resp.getStatusCode() == 1) {
        this->started = true;
      }
      emit this->authenticatheResponse(this->started);

  });
  this->client.sendAuthRequest(1001, username, User::hashSHA256(password));
  
  return true;
}

bool Model::deleteUser(
    const std::string& username) {
  return 1 /*this->usersManager.deleteUser(username)*/;
}

bool Model::deleteUser(
    const User& user) {
  return 1 /*this->usersManager.deleteUser(user.getUsername())*/;
}

bool Model::updateUser(
    const std::string& username, const User& updatedUser) {
  return 1 /*this->usersManager.updateUser(username, updatedUser)*/;
}

bool Model::saveUser(
    const QString &username, const QString &password, const QString &rol) {
  User newUser(username.toStdString(), rol.toStdString());
  newUser.setPassword(password.toStdString());
  return 1; /*this->usersManager.saveUser(newUser);*/
}
