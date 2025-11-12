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
  
  qDebug() << "[QtUDPClient] Sending datagram";
  
  
  // Send datagram to the configured server address and port
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(
        QString("[QtUDPClient] Failed to send AUTH_REQUEST: %1")
            .arg(udpSocket->errorString())
    );
  } else {
    qDebug() << "[QtUDPClient] Sent AUTH_REQUEST datagram ("
             << sent << " bytes)"
             << "to" << serverAddress.toString()
             << ":" << serverPort;
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

void QtUDPClient::handleReadyRead() {
  while (udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
    QHostAddress sender;
    quint16 senderPort;
    
    udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    
    emit datagramReceived(datagram, sender, senderPort);
    
    const int size = datagram.size();
    
    if (size == static_cast<int>(AuthResponse::MESSAGE_SIZE + AuthResponse::TOKEN_SIZE + 3)) {
      this->handleAuthResponseDatagram(datagram, sender, senderPort);
    } else if (size == static_cast<int>(sizeof(SensorData))) {
      // Procesar datos de sensor
      this->handleSensorDataDatagram(datagram, sender, senderPort);
    } else {
      qDebug() << "[QtUDPClient] Received datagram of unexpected size:" << size << "bytes from" << sender.toString();
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
    
    if (response.getStatusCode() == 1) {
      qDebug() << "[QtUDPClient] Authentication successful, sending CONNECT_REQUEST...";
      this->sendConnectRequest(this->sessionId);
    }
    
  } catch (const std::exception &ex) {
    emit errorOccurred(QString("[QtUDPClient] Invalid AuthResponse: %1").arg(ex.what()));
  }
}

void QtUDPClient::handleSensorDataDatagram(
    const QByteArray &datagram, const QHostAddress &sender,quint16 senderPort) {
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

void QtUDPClient::handleUnknownDatagram(
    const QByteArray &datagram, const QHostAddress &sender, quint16 senderPort) {
  qDebug() << "[QtUDPClient] Received unknown datagram ("
           << datagram.size() << " bytes)"
           << "from" << sender.toString() << ":" << senderPort;
}
