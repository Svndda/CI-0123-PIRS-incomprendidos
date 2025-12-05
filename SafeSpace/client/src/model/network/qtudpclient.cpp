#include "qtudpclient.h"
#include "connectrequest.h"
#include "network/GetSensorDataRequest.h"
#include "network/GetSystemUsersRequest.h"
#include "sensordata.h"

QtUDPClient::QtUDPClient(
    const QString &serverIp, quint16 serverPort, QObject *parent)
    : QObject(parent),
    udpSocket(new QUdpSocket(this)),
    serverAddress(QHostAddress(serverIp)),
    serverPort(serverPort) {
  
  if (!serverAddress.isNull()) {
    qDebug() << "[QtUDPClient] Initialized for"
             << serverIp << ":" << serverPort;
  } else {
    emit errorOccurred("[QtUDPClient] Invalid server IP address");
  }
  
  // Connect signal for incoming datagrams
  this->connect(
      this->udpSocket,
      &QUdpSocket::readyRead,
      this,
      &QtUDPClient::handleReadyRead
      );
}

QtUDPClient::~QtUDPClient() {
  if (udpSocket) {
    udpSocket->close();
    qDebug() << "[QtUDPClient] Socket closed.";
  }
}

void QtUDPClient::sendAuthRequest(std::uint16_t sessionId,
                                  const std::string& username,
                                  const std::string& password) {
  // Construct the binary payload
  AuthRequest request(sessionId, username, password);
  auto buffer = request.toBuffer();
  
  // Convert std::array to QByteArray for transmission
  QByteArray datagram(
      reinterpret_cast<const char*>(buffer.data()),
      static_cast<int>(buffer.size())
      );
  
  qDebug() << "[QtUDPClient] Sending AUTH_REQUEST datagram";
  
  // Send datagram to the configured server address and port
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(
        QString("[QtUDPClient] Failed to send AUTH_REQUEST: %1")
            .arg(udpSocket->errorString())
        );
  } else {
    qDebug() << "[QtUDPClient] Sent AUTH_REQUEST datagram ("
             << sent << " bytes) to" 
             << serverAddress.toString() << ":" << serverPort;
  }
}

void QtUDPClient::sendConnectRequest(std::uint16_t sessionId) {
  ConnectRequest request(sessionId, 0, 0);
  
  auto buffer = request.toBuffer();
  
  QByteArray datagram(reinterpret_cast<const char*>(buffer.data()),
                      static_cast<int>(buffer.size()));
  
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(QString("[QtUDPClient] Failed to send CONNECT_REQUEST: %1")
                           .arg(udpSocket->errorString()));
  } else {
    qDebug() << "[QtUDPClient] Sent CONNECT_REQUEST (" << sent << " bytes)";
  }
}

void QtUDPClient::sendGetSensorDataRequest(std::uint16_t sessionId, 
                                           std::uint16_t sensorId, 
                                           const Token16& token) {
  GetSensorDataRequest request;
  request.sessionId = sessionId;
  request.sensorId = sensorId;
  request.token = token;
  
  // Convert to bytes
  auto bytes = request.toBytes();
  
  QByteArray datagram(reinterpret_cast<const char*>(bytes.data()),
                      static_cast<int>(bytes.size()));
  
  qDebug() << "[QtUDPClient] Sending GET_SENSOR_DATA_REQUEST for sensorId=" 
           << sensorId << " sessionId=" << sessionId;
  
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(QString("[QtUDPClient] Failed to send GET_SENSOR_DATA_REQUEST: %1")
                           .arg(udpSocket->errorString()));
  } else {
    qDebug() << "[QtUDPClient] Sent GET_SENSOR_DATA_REQUEST (" 
             << sent << " bytes)";
  }
}

// void QtUDPClient::sendDeleteSensorDataRequest(std::uint16_t sessionId, 
//                                               std::uint16_t sensorId, 
//                                               const Token16& token) {
//   DeleteSensorDataRequest request;
//   request.sessionId = sessionId;
//   request.sensorId = sensorId;
//   request.token = token;
  
//   // Convert to bytes
//   auto bytes = request.toBytes();
  
//   QByteArray datagram(reinterpret_cast<const char*>(bytes.data()),
//                       static_cast<int>(bytes.size()));
  
//   qDebug() << "[QtUDPClient] Sending DELETE_SENSOR_DATA_REQUEST for sensorId=" 
//            << sensorId << " sessionId=" << sessionId;
  
//   qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
//   if (sent < 0) {
//     emit errorOccurred(QString("[QtUDPClient] Failed to send DELETE_SENSOR_DATA_REQUEST: %1")
//                            .arg(udpSocket->errorString()));
//   } else {
//     qDebug() << "[QtUDPClient] Sent DELETE_SENSOR_DATA_REQUEST (" 
//              << sent << " bytes)";
//   }
// }

void QtUDPClient::sendGetSystemUsersRequest(std::uint16_t sessionId) {
  // Crear el buffer directamente
  QByteArray datagram = GetSystemUsersRequest::createForSending(sessionId);
  
  qDebug() << "[QtUDPClient] Sending GET_SYSTEM_USERS_REQUEST with sessionId=" 
           << sessionId << "size:" << datagram.size() << "bytes";
  
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(QString("[QtUDPClient] Failed to send GET_SYSTEM_USERS_REQUEST: %1")
                           .arg(udpSocket->errorString()));
  } else {
    qDebug() << "[QtUDPClient] Sent GET_SYSTEM_USERS_REQUEST (" 
             << sent << " bytes)";
  }
}

void QtUDPClient::handleReadyRead() {
  while (udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
    QHostAddress sender;
    quint16 senderPort;
    
    udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    
    emit datagramReceived(datagram, sender, senderPort);
    
    const int size = datagram.size();
    
    // Determinar el tipo de mensaje basado en el primer byte (msgId)
    if (size > 0) {
      uint8_t msgId = static_cast<uint8_t>(datagram[0]);
    
      if (size == static_cast<int>(AuthResponse::MESSAGE_SIZE + AuthResponse::TOKEN_SIZE + 3)) {
        handleAuthResponseDatagram(datagram, sender, senderPort);
      } else if (msgId == GetSensorDataResponse::MSG_ID) {
        handleGetSensorDataResponse(datagram, sender, senderPort);
        
      } else if (msgId == 0x21) {
        handleGetSystemUsersResponse(datagram, sender, senderPort);
      }
      else if (size == static_cast<int>(sizeof(SensorData))) {
        handleSensorDataDatagram(datagram, sender, senderPort);
      } else {
        handleUnknownDatagram(datagram, sender, senderPort);
      }
      //   handleDeleteSensorDataResponse(datagram, sender, senderPort);
      
    }
  }
}

void QtUDPClient::handleAuthResponseDatagram(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  
  try {
    std::array<uint8_t, 51> buffer{};
    std::memcpy(buffer.data(), datagram.constData(), buffer.size());
    
    AuthResponse response;
    response.setSessionId((buffer[0] << 8) | buffer[1]);
    response.setStatusCode(buffer[2]);
    response.setMessage(std::string(reinterpret_cast<char*>(buffer.data() + 3),
                                    AuthResponse::MESSAGE_SIZE));
    response.setSessionToken(std::string(reinterpret_cast<char*>(buffer.data() + 3 + AuthResponse::MESSAGE_SIZE),
                                         AuthResponse::TOKEN_SIZE));
    
    this->sessionId = response.getSessionId();
    
    emit authResponseReceived(response);
    
    qDebug() << "[QtUDPClient] Received AuthResponse:"
             << "sessionId=" << response.getSessionId()
             << "status=" << response.getStatusCode()
             << "message=" << QString::fromStdString(response.getMessage())
             << "token=" << QString::fromStdString(response.getSessionToken());
    
    // if (response.getStatusCode() == 1) {
    //   qDebug() << "[QtUDPClient] Authentication successful, sending CONNECT_REQUEST...";
    //   this->sendConnectRequest(this->sessionId);
    // }
    
  } catch (const std::exception &ex) {
    emit errorOccurred(QString("[QtUDPClient] Invalid AuthResponse: %1").arg(ex.what()));
  }
}

void QtUDPClient::handleSensorDataDatagram(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  try {
    SensorData sensor{};
    std::memcpy(&sensor, datagram.constData(), sizeof(SensorData));
    
    emit sensorDataReceived(sensor);
    
    qDebug() << "[QtUDPClient] Received SensorData:"
             << "Temp=" << sensor.temperature
             << "Dist=" << sensor.distance
             << "from" << sender.toString() << ":" << senderPort;
  } catch (const std::exception &ex) {
    emit errorOccurred(QString("[QtUDPClient] Invalid SensorData: %1").arg(ex.what()));
  }
}

void QtUDPClient::handleGetSensorDataResponse(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  try {
    // Convertir QByteArray a vector de bytes
    std::vector<uint8_t> buffer(datagram.size());
    std::memcpy(buffer.data(), datagram.constData(), datagram.size());
    
    // Parsear la respuesta
    GetSensorDataResponse response = GetSensorDataResponse::fromBytes(buffer.data(), buffer.size());
    
    qDebug() << "[QtUDPClient] Received GetSensorDataResponse:"
             << "sessionId=" << response.sessionId
             << "status=" << static_cast<int>(response.status)
             << "payload size=" << response.payload.size() << "bytes";
    
    emit getSensorDataResponseReceived(response);
    
  } catch (const std::exception &ex) {
    emit errorOccurred(QString("[QtUDPClient] Invalid GetSensorDataResponse: %1").arg(ex.what()));
  }
}

// void QtUDPClient::handleDeleteSensorDataResponse(
//     const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
//   try {
//     // Convertir QByteArray a vector de bytes
//     std::vector<uint8_t> buffer(datagram.size());
//     std::memcpy(buffer.data(), datagram.constData(), datagram.size());
    
//     // Parsear la respuesta
//     DeleteSensorDataResponse response = DeleteSensorDataResponse::fromBytes(buffer.data(), buffer.size());
    
//     qDebug() << "[QtUDPClient] Received DeleteSensorDataResponse:"
//              << "sessionId=" << response.sessionId
//              << "status=" << static_cast<int>(response.status);
    
//     emit deleteSensorDataResponseReceived(response);
    
//   } catch (const std::exception &ex) {
//     emit errorOccurred(QString("[QtUDPClient] Invalid DeleteSensorDataResponse: %1").arg(ex.what()));
//   }
// }

void QtUDPClient::handleGetSystemUsersResponse(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  
  try {
    // parse safely (does validation inside)
    GetSystemUsersResponse resp = GetSystemUsersResponse::parseFromQByteArray(datagram);
    
    // Build QStrings for logging/UI
    const QString username = QString::fromStdString(resp.getTrimmedUsername());
    const QString group = QString::fromStdString(resp.getTrimmedGroup());
    
    qDebug() << "[QtUDPClient] Received GetSystemUsersResponse:"
             << " sessionId=" << resp.sessionId
             << " totalUsers=" << resp.totalUsers
             << " currentIndex=" << resp.currentIndex
             << " username=" << username
             << " group=" << group
             << " permissions=" << static_cast<int>(resp.permissions);
    
    emit systemUsersResponseReceived(resp);
    
  } catch (const std::exception &ex) {
    qWarning() << "[QtUDPClient] Invalid GetSystemUsersResponse:" << ex.what();
    emit errorOccurred(QString("[QtUDPClient] Invalid GetSystemUsersResponse: %1")
                           .arg(ex.what()));
  }
}

void QtUDPClient::handleUnknownDatagram(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  qDebug() << "[QtUDPClient] Received unknown datagram ("
           << datagram.size() << " bytes)"
           << "from" << sender.toString() << ":" << senderPort
           << "First byte: 0x" << QString::number(static_cast<uint8_t>(datagram[0]), 16);
}
