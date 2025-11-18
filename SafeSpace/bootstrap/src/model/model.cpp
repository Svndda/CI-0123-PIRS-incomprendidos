// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include <vector>
#include <limits>

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include "FileSystem.h"
#include "model.h"

Model::Model()
  : client("172.17.0.70", 8080, this) {
  
  if (!loadNodesFromFile()) {
    qWarning() << "[Model] No se pudo cargar configuración de nodos, usando valores por defecto";
    
    // fallback
    nodes = {
        {0, "SafeSpaceServer",   "172.17.0.10", 8000, "Apagado"},
        {1, "Proxy",   "172.17.0.20", 9000, "Apagado"},
        {2, "Almacenamiento",       "172.17.0.70", 8003, "Apagado"},      
        {3, "Intermediario",   "172.17.0.90", 8554, "Apagado"},
        {4, "Autenticacion",    "172.17.0.35", 7999, "Apagado"},
        {5, "Arduino",       "172.17.0.120", 7600, "Apagado"},
        {6, "Eventos Criticos",     "172.17.0.110", 8080, "Apagado"}
    };
  }
  
  connect(&client, &QtUDPClient::requestSent,
          this, [this](QString type, QString detail, QByteArray raw){
            NetworkEvent evt;
            evt.direction = "REQUEST";
            evt.type = type;
            evt.detail = detail;
            evt.rawBytes = raw;
            evt.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            addNetworkEvent(evt);
          });
  
  // Registrar RESPONSES
  connect(&client, &QtUDPClient::responseReceived,
          this, [this](QString type, QString detail, QByteArray raw){
            NetworkEvent evt;
            evt.direction = "RESPONSE";
            evt.type = type;
            evt.detail = detail;
            evt.rawBytes = raw;
            evt.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
            
            addNetworkEvent(evt);
          });
  
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
  bool nodeExists = false;
  for (const auto& node : nodes) {
    if (node.id == nodeId) {
      nodeExists = true;
      break;
    }
  }
  
  if (!nodeExists) {
    qWarning() << "[Model] Intento de ejecutar nodo inexistente:" << nodeId;
    return;
  }
  
  this->client.sendRunNodeRequest(nodeId);
}

void Model::stopNode(uint8_t nodeId) {
  bool nodeExists = false;
  for (const auto& node : nodes) {
    if (node.id == nodeId) {
      nodeExists = true;
      break;
    }
  }
  
  if (!nodeExists) {
    qWarning() << "[Model] Intento de detener nodo inexistente:" << nodeId;
    return;
  }
  this->client.sendStopNodeRequest(nodeId);
}

void Model::addNetworkEvent(const NetworkEvent& evt) {
  this->networkLog.push_back(evt);
  emit this->networkEventLogged(evt);
}

bool Model::loadNodesFromFile(const QString& filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "[Model] No se pudo abrir el archivo:" << filename;
    return false;
  }
  
  nodes.clear(); // Limpiar lista existente
  
  QTextStream in(&file);
  int lineNumber = 0;
  int loadedCount = 0;
  
  while (!in.atEnd()) {
    lineNumber++;
    QString line = in.readLine().trimmed();
    
    // Saltar líneas vacías o comentarios
    if (line.isEmpty() || line.startsWith('#')) {
      continue;
    }
    
    // Dividir por pipe '|'
    QStringList parts = line.split('|');
    if (parts.size() != 5) {
      qWarning() << "[Model] Formato inválido en línea" << lineNumber << ":" << line;
      continue;
    }
    
    // Validar y convertir campos
    bool ok;
    int id = parts[0].toInt(&ok);
    if (!ok) {
      qWarning() << "[Model] ID inválido en línea" << lineNumber << ":" << parts[0];
      continue;
    }
    
    int port = parts[3].toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
      qWarning() << "[Model] Puerto inválido en línea" << lineNumber << ":" << parts[3];
      continue;
    }
    
    // Crear y agregar nodo
    NodeInfo node;
    node.id = id;
    node.name = parts[1].trimmed();
    node.ip = parts[2].trimmed();
    node.port = port;
    node.status = parts[4].trimmed();
    
    nodes.append(node);
    loadedCount++;
  }
  
  file.close();
  
  qInfo() << "[Model] Cargados" << loadedCount << "nodos desde" << filename;
  return !nodes.isEmpty();
}

NodeInfo Model::getNodeById(int nodeId) const {
  for (const auto& node : nodes) {
    if (node.id == nodeId) {
      return node;
    }
  }
  
  return NodeInfo{};
}

bool Model::updateNodeStatus(int nodeId, const QString& status) {
  for (auto& node : nodes) {
    if (node.id == nodeId) {
      node.status = status;
      qDebug() << "[Model] Estado actualizado - Nodo" << nodeId << "->" << status;
      return true;
    }
  }
  return false;
}
