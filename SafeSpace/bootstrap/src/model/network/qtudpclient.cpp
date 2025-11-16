#include "qtudpclient.h"
#include "connectrequest.h"
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

void QtUDPClient::sendRunNodeRequest(uint8_t nodeId) {
  // Create request object
  RunNodeRequest request(nodeId);
  
  // Serialize to 2-byte array
  auto bytes = request.toBytes();
  
  // Convert to QByteArray
  QByteArray datagram(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  
  // Send datagram
  qint64 sent = udpSocket->writeDatagram(
      datagram, serverAddress, serverPort
      );
  
  if (sent < 0) {
    emit errorOccurred("[QtUDPClient] Failed to send RunNodeRequest: " 
                       + udpSocket->errorString());
  } else {
    qDebug() << "[QtUDPClient] Sent RunNodeRequest for node" << nodeId;
  }
}

void QtUDPClient::sendStopNodeRequest(uint8_t nodeId) {
  // Create STOP_NODE datagram
  StopNodeRequest request(nodeId);
  
  auto bytes = request.toBytes();
  
  QByteArray datagram(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  
  qint64 sent = udpSocket->writeDatagram(
      datagram, serverAddress, serverPort
      );
  
  if (sent < 0) {
    emit errorOccurred("[QtUDPClient] Failed to send StopNodeRequest: " 
                       + udpSocket->errorString());
  } else {
    qDebug() << "[QtUDPClient] Sent StopNodeRequest for node" << nodeId;
  }
}

// void QtUDPClient::sendConnectRequest(std::uint16_t sessionId) {
//   ConnectRequest request(sessionId, 0, 0);
  
//   auto buffer = request.toBuffer();
  
//   QByteArray datagram(reinterpret_cast<const char*>(buffer.data()),
//                       static_cast<int>(buffer.size()));
  
//   qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
//   if (sent < 0) {
//     emit errorOccurred(QString("[QtUDPClient] Failed to send CONNECT_REQUEST: %1")
//                            .arg(udpSocket->errorString()));
//   } else {
//     qDebug() << "[QtUDPClient] Sent CONNECT_REQUEST (" << sent << " bytes)";
//   }
// }

void QtUDPClient::handleReadyRead() {
  while (udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    QHostAddress sender;
    quint16 senderPort;
    
    datagram.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
    udpSocket->readDatagram(
        datagram.data(), datagram.size(),
        &sender, &senderPort
    );
    
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(datagram.data());        
    const int size = datagram.size();
    
    // RUN_NODE_RESPONSE → exactly 3 bytes
    if (size == 3 && raw[0] == 0x7c) {
      try {
        RunNodeResponse resp = RunNodeResponse::fromBytes(raw, size);
        emit runNodeResponseReceived(resp);
        continue;
      } catch (...) {
        emit errorOccurred("[QtUDPClient] Invalid RunNodeResponse");
      }
    }
    
    // STOP_NODE_RESPONSE → exactly 3 bytes
    if (size == 3 && raw[0] == 0x7e) {
      try {
        StopNodeResponse resp = StopNodeResponse::fromBytes(raw, size);
        emit stopNodeResponseReceived(resp);
        continue;
      } catch (...) {
        emit errorOccurred("[QtUDPClient] Invalid StopNodeResponse");
      }
    }
    
    // if (size == static_cast<int>(AuthResponse::MESSAGE_SIZE + AuthResponse::TOKEN_SIZE + 3)) {
    //   this->handleAuthResponseDatagram(datagram, sender, senderPort);
    // } else {
    //   qDebug() << "[QtUDPClient] Received datagram of unexpected size:" << size << "bytes from" << sender.toString();
    // }
  }
}

// void QtUDPClient::handleAuthResponseDatagram(
//   const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  
//   try {
//     std::array<uint8_t, 51> buffer{};
//     std::memcpy(buffer.data(), datagram.constData(), buffer.size());
    
//     AuthResponse response;
//     response.setSessionId((buffer[0] << 8) | buffer[1]);
//     response.setStatusCode(buffer[2]);
//     response.setMessage(std::string(reinterpret_cast<char*>(buffer.data() + 3),
//                                     AuthResponse::MESSAGE_SIZE));
//     response.setSessionToken(std::string(reinterpret_cast<char*>(buffer.data() + 3 + AuthResponse::MESSAGE_SIZE),
//                                          AuthResponse::TOKEN_SIZE));
    
//     this->sessionId = response.getSessionId();
//     emit authResponseReceived(response);
    
//     qDebug() << "[QtUDPClient] Received AuthResponse:"
//              << "sessionId=" << response.getSessionId()
//              << "status=" << response.getStatusCode()
//              << "message=" << QString::fromStdString(response.getMessage())
//              << "token=" << QString::fromStdString(response.getSessionToken());
    
//     if (response.getStatusCode() == 1) {
//       qDebug() << "[QtUDPClient] Authentication successful, sending CONNECT_REQUEST...";
//       this->sendConnectRequest(this->sessionId);
//     }
    
//   } catch (const std::exception &ex) {
//     emit errorOccurred(QString("[QtUDPClient] Invalid AuthResponse: %1").arg(ex.what()));
//   }
// }

void QtUDPClient::handleUnknownDatagram(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  qDebug() << "[QtUDPClient] Received unknown datagram ("
           << datagram.size() << " bytes)"
           << "from" << sender.toString() << ":" << senderPort;
}
