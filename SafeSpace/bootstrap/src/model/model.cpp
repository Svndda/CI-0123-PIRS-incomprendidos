// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include <vector>
#include <limits>

#include <QDebug>
#include "FileSystem.h"
#include "model.h"

Model::Model()
  : client("172.17.0.70", 8080, this) {
  
  this->connect(&this->client, &QtUDPClient::runNodeResponseReceived,
      this, [this](const RunNodeResponse& resp) {
        qInfo() << "[Model] RunNodeResponse received: node"
                << resp.nodeId() << "status" << resp.status();
        
        emit runNodeResult(resp.nodeId(), resp.status());
      }
  );
  
  this->connect(&this->client, &QtUDPClient::stopNodeResponseReceived,
      this, [this](const StopNodeResponse& resp) {
        qInfo() << "[Model] StopNodeResponse received: node"
                << resp.nodeId() << "status" << resp.status();
        
        emit stopNodeResult(resp.nodeId(), resp.status());
      }
  );
}

Model& Model::getInstance() {
  // Creates an static instance of the POS MODEL.
  static Model instance;
  return instance;
}

bool Model::authenticate(
    const std::string& username, const std::string& password) {
  // return this->usersManager.authenticate(username, password);
  std::cout << "Autenticando usuarios" << std::endl;
  if (username == this->user.getUsername()
      && this->user.verifyPassword(password)) {
    qInfo() << "Credenciales aceptadas";
    this->started = true;
    return true;
  }
  qInfo() << "Credenciales denegadas";  
  return false;
}

void Model::runNode(uint8_t nodeId) {
  this->client.sendRunNodeRequest(nodeId);
}

void Model::stopNode(uint8_t nodeId) {
  this->client.sendStopNodeRequest(nodeId);
}


