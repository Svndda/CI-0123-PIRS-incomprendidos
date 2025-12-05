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
  : client("172.17.0.70", 8083, this) {
    
  this->connect(
      &this->client, &QtUDPClient::sensorDataReceived,
      this, [this](const SensorData& data) {
        qInfo() << "[Model] SensorData received:" << data.distance;        
        this->sensorsData.emplace_back(data);
        emit this->sensorDataReceived(data);
      });
  
  QObject::connect(
      &client,
      &QtUDPClient::systemUsersResponseReceived,
      this,
      &Model::onSystemUsersResponseReceived
      );
  
  
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
          this->sessionId = resp.getSessionId();
          this->token = Token16(resp.getSessionToken());
          this->client.sendConnectRequest(resp.getSessionId());
          this->client.sendGetSystemUsersRequest(this->sessionId);
          this->client.sendGetSensorDataRequest(this->sessionId, 0, this->token);
        }
        emit this->authenticatheResponse(this->started);
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
  this->user = User(username, User::hashSHA256(password), "guest", 4, 0, false);
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

void Model::onSystemUsersResponseReceived(
    const GetSystemUsersResponse& response) {
  
  // Clear vector when first packet arrives
  if (response.currentIndex == 0) {
    systemUsers.clear();
  }
  
  // Parse raw UDP response to User entity
  User user(response.username, "", response.group, response.permissions, 0, false);
  if (this->user.getUsername() == response.username)
    this->user = user;
  std::cout << "Usuario matched" <<std::endl;
  
  systemUsers.push_back(user);
  
  // If last packet → emit full list
  if (response.currentIndex == response.totalUsers) {
    emit systemUsersReceived(systemUsers);
  }
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

void Model::reset() {
  // --- Stop the model execution state ---
  // This flag controls whether the system is considered authenticated
  this->started = false;
  
  this->sessionId = 0;
  

  this->token = Token16();
  

  this->user = User();
  

  this->systemUsers.clear();
  this->systemUsers.shrink_to_fit();
  
  this->sensorsData.clear();
  this->sensorsData.shrink_to_fit();
  
  
  qDebug() << "[Model] Full system reset completed.";
}

