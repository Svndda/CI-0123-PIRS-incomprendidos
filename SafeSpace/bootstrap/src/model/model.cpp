// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include <qfontinfo.h>
#include <vector>
#include <limits>

#include <QDebug>
#include <QFile>
#include <QDateTime>
#include "FileSystem.h"
#include "model.h"

Model::Model() {
  
  if (!loadFromFile()) {
    qWarning() << "[Model] No se pudo cargar configuración de nodos, usando valores por defecto";
    
    this->bootstraps.push_back(
      {
          0,
          "B-00",
          std::make_unique<QtUDPClient>("172.17.0.70", 8080, this),
          "Apagado",
          {
              {0, "SafeSpaceServer", "127.0.0.1", 6000, "Apagado"}
          }
      }
    );
    
    this->bootstraps.push_back(
      // Bootstrap ID 1: "B-01"
      {
          1,
          "B-01",
          std::make_unique<QtUDPClient>("172.17.0.70", 8081, this),
          "Apagado",
          {
              {2, "Almacenamiento", "0.0.0.0", 9001, "Apagado"},
              {6, "Eventos Criticos", "0.0.0.0", 6001, "Apagado"}
          }
      }
    );
    this->bootstraps.push_back(  
      {
          2,
          "B-02",
          std::make_unique<QtUDPClient>("172.17.0.70", 8082, this),
          "Apagado",
          {
              {1, "Proxy", "0.0.0.0", 9000, "Apagado"},
              {4, "Autenticacion", "0.0.0.0", 7000, "Apagado"}
          }
      }
    );
    this->bootstraps.push_back(
      {
          3,
          "B-03",
          std::make_unique<QtUDPClient>("172.17.0.70", 8083, this),
          "Apagado",
          {
              {3, "Intermediario", "0.0.0.0", 9002, "Apagado"},
              {5, "Arduino", "127.0.0.1", 9002, "Apagado"}
          }
      }
    );
  }
  
  for (auto& bootstrap : this->bootstraps) {
    for (const auto& node : bootstrap.nodes) {
      this->nodes.push_back(node);
    }
    this->connectClientSignals(bootstrap.client.get(), bootstrap.id);
  }
  
}

Model& Model::getInstance() {
  // Creates an static instance of the POS MODEL.
  static Model instance;
  return instance;
}

void Model::connectClientSignals(QtUDPClient* client, int nodeId) {
  connect(client, &QtUDPClient::requestSent,
          this, [this, nodeId](QString type, QString detail, QByteArray raw){
            NetworkEvent evt;
            evt.direction = "REQUEST";
            evt.type = type;
            evt.detail = detail;
            evt.rawBytes = raw;
            evt.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
            evt.nodeId = nodeId; 
            addNetworkEvent(evt);
          });
  
  connect(client, &QtUDPClient::responseReceived,
          this, [this, nodeId](QString type, QString detail, QByteArray raw){
            NetworkEvent evt;
            evt.direction = "RESPONSE";
            evt.type = type;
            evt.detail = detail;
            evt.rawBytes = raw;
            evt.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
            evt.nodeId = nodeId; 
            addNetworkEvent(evt);
          });
  
  this->connect(client, &QtUDPClient::runNodeResponseReceived,
                this, [this](const RunNodeResponse& resp) {
                  qInfo() << "[Model] RunNodeResponse received: node"
                          << resp.nodeId() << "status" << resp.status();
                  emit runNodeResult(resp.nodeId(), resp.status());
                }
                );
  
  this->connect(client, &QtUDPClient::stopNodeResponseReceived,
                this, [this](const StopNodeResponse& resp) {
                  qInfo() << "[Model] StopNodeResponse received: node"
                          << resp.nodeId() << "status" << resp.status();
                  emit stopNodeResult(resp.nodeId(), resp.status());
                }
                );
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
  
  for (auto& bootstrap : this->bootstraps) {
    const auto& client = bootstrap.client;
    for (const auto& node : bootstrap.nodes) {
      if (node.id == nodeId) {
        client->sendRunNodeRequest(nodeId);        
      }
    }
  }
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
  
  for (auto& bootstrap : this->bootstraps) {
    const auto& client = bootstrap.client;
    for (const auto& node : bootstrap.nodes) {
      if (node.id == nodeId) {
        client->sendStopNodeRequest(nodeId);        
      }
    }
  }}

void Model::addNetworkEvent(const NetworkEvent& evt) {
  this->networkLog.push_back(evt);
  emit this->networkEventLogged(evt);
}

bool Model::loadFromFile(const QString& filename) {
  QString executablePath = QCoreApplication::applicationDirPath();
  QString absoluteFilePath = executablePath + QDir::separator() + filename;
  
  if (!QFileInfo::exists(absoluteFilePath)) {
    qWarning() << "[Model] Archivo de configuración NO encontrado en:" << absoluteFilePath;
    qWarning() << "[Model] Intentando crear el archivo con valores por defecto...";
    
    if (!generateDefaultConfigFile(absoluteFilePath)) {
      return false;
    }
  }
  
  QFile file(absoluteFilePath); 
  
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "[Model] Error fatal al abrir el archivo consolidado (¡después de crearlo!):" << absoluteFilePath;
    return false;
  }
  
  std::unordered_map<int, Bootstrap> tempBootstraps; 
  
  QTextStream in(&file);
  int lineNumber = 0;
  int loadedCount = 0;
  
  while (!in.atEnd()) {
    lineNumber++;
    QString line = in.readLine().trimmed();
    
    if (line.isEmpty() || line.startsWith('#')) {
      continue;
    }
    
    QStringList parts = line.split('|');
    if (parts.size() != 8) {
      qWarning() << "[Model] Formato inválido en línea" << lineNumber << ":" << line;
      continue;
    }
    
    bool ok_b_id, ok_b_port, ok_n_id, ok_n_port;
    
    // Datos del Bootstrap
    int b_id = parts[0].toInt(&ok_b_id);
    QString b_name = parts[1].trimmed();
    QString b_ip = parts[2].trimmed();
    int b_port = parts[3].toInt(&ok_b_port);
    
    // Datos del Nodo
    int n_id = parts[4].toInt(&ok_n_id);
    QString n_ip = parts[5].trimmed();
    int n_port = parts[6].toInt(&ok_n_port);
    QString n_name = parts[7].trimmed();
    
    if (!ok_b_id || !ok_b_port || !ok_n_id || !ok_n_port) {
      qWarning() << "[Model] Error de conversión de ID/Puerto en línea" << lineNumber << ".";
      continue;
    }
    
    NodeInfo newNode = {n_id, n_name, n_ip, n_port, "Apagado"};
    
    if (tempBootstraps.find(b_id) == tempBootstraps.end()) {
      tempBootstraps.emplace(b_id, Bootstrap{b_id,
       b_name,
       std::make_unique<QtUDPClient>(b_ip, b_port, this),
       "Apagado",
       std::vector<NodeInfo>{newNode}
      });
    } else {
      tempBootstraps.at(b_id).nodes.push_back(newNode);
    }
    
    loadedCount++;
  }
  
  file.close();
  
  this->bootstraps.clear();
  for (auto& pair : tempBootstraps) {
    this->bootstraps.push_back(std::move(pair.second));
  }
  
  qInfo() << "[Model] Cargados" << tempBootstraps.size() << "bootstraps y" << loadedCount << "nodos asociados desde" << filename;
  
  return !this->bootstraps.empty();
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

bool Model::generateDefaultConfigFile(const QString& absoluteFilePath) {
  QFile file(absoluteFilePath);
  
  // Abrir para escribir, truncando si ya existe.
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    qWarning() << "[Model] ERROR: No se pudo crear o abrir el archivo para escritura:" << absoluteFilePath;
    return false;
  }
  
  QTextStream out(&file);
  out << "# Archivo de Configuracion Consolidada de Bootstraps y Nodos\n";
  out << "# Formato: ID_B|Nombre_B|IP_B|Puerto_B|ID_N|IP_N|Puerto_N|Nombre_N\n\n";
  
  
  // B-00
  out << "0|B-00|172.17.0.70|8080|0|127.0.0.1|6000|SafeSpaceServer\n";
  
  // B-01
  out << "1|B-01|172.17.0.70|8081|2|0.0.0.0|9001|Almacenamiento\n";
  out << "1|B-01|172.17.0.70|8081|6|0.0.0.0|6001|Eventos Criticos\n";
  
  // B-02
  out << "2|B-02|172.17.0.70|8082|1|0.0.0.0|9000|Proxy\n";
  out << "2|B-02|172.17.0.70|8082|4|0.0.0.0|7000|Autenticacion\n";
  
  // B-03
  out << "3|B-03|172.17.0.70|8083|3|0.0.0.0|9002|Intermediario\n";
  out << "3|B-03|172.17.0.70|8083|5|127.0.0.1|9002|Arduino\n";
  
  file.close();
  qInfo() << "[Model] Archivo de configuración por defecto creado en:" << absoluteFilePath;
  return true;
}
