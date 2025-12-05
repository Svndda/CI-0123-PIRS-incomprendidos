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
  
  this->connect(
      &this->client, &QtUDPClient::getSensorDataResponseReceived,
      this, &Model::onGetSensorDataResponseReceived
      );
  
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

void Model::onGetSensorDataResponseReceived(const GetSensorDataResponse &response) {
  // --- Logging initial information ---
  qInfo() << "[Model] Processing GetSensorDataResponse:"
          << "sessionId=" << response.sessionId
          << "status=" << static_cast<int>(response.status)
          << "payload size=" << response.payload.size();
  
  if (response.status != 0) {
    qWarning() << "[Model] GetSensorDataResponse returned error status:" << response.status;
    return;
  }
  
  // Clear previous data before processing new batch
  this->sensorsData.clear();
  
  size_t offset = 0;
  const size_t payloadSize = response.payload.size();
  const size_t sensorDataSize = sizeof(float) * 6; // distance, temp, pres, seaPres, alt, realAlt
  
  // --- Process each SensorData chunk ---
  while (offset + sensorDataSize <= payloadSize) {
    SensorData sensor;
    
    // Pointer to current position in payload
    const uint8_t* ptr = response.payload.data() + offset;
    
    // Safe conversion: memcpy ensures proper alignment
    std::memcpy(&sensor.distance, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&sensor.temperature, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&sensor.pressure, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&sensor.sealevelPressure, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&sensor.altitude, ptr, sizeof(float)); ptr += sizeof(float);
    std::memcpy(&sensor.realAltitude, ptr, sizeof(float)); ptr += sizeof(float);
    
    // Add sensor to local storage vector
    this->sensorsData.push_back(sensor);
    
    // // Emit signal for UI or other subscribers
    // emit this->sensorDataReceived(sensor);
    
    // Debug logging for each sensor
    qDebug() << "[Model] SensorData added:"
             << "distance=" << sensor.distance
             << "temperature=" << sensor.temperature
             << "pressure=" << sensor.pressure
             << "sealevelPressure=" << sensor.sealevelPressure
             << "altitude=" << sensor.altitude
             << "realAltitude=" << sensor.realAltitude;
    
    // Move offset to next SensorData block
    offset += sensorDataSize;
  }
  
  // --- Check if payload had leftover bytes ---
  if (offset != payloadSize) {
    qWarning() << "[Model] Payload size mismatch: leftover bytes detected."
               << "Processed=" << offset
               << "Total=" << payloadSize;
  }
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

