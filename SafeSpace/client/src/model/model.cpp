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
  : client("10.1.35.23", 8081, this) {
  
  this->connect(
      &this->client, &QtUDPClient::sensorDataReceived,
      this, [this](const SensorData& data) {
        qInfo() << "[Model] SensorData received:" << data.distance;        
        this->sensorsData.emplace_back(data);
        emit this->sensorDataReceived(data);
      });
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

void Model::authenticate(
    const std::string& username, const std::string& password) {
  // return this->usersManager.authenticate(username, password);
    std::cout << "Autentiando usuarios" << std::endl;
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
        this->client.sendConnectRequest(resp.getSessionId());
      }
      emit this->authenticatheResponse(this->started);
  });
  this->client.sendAuthRequest(1001, username, User::hashSHA256(password));
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
