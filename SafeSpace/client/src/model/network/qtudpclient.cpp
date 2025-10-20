#include "QtUDPClient.h"

QtUDPClient::QtUDPClient(const QString &serverIp, quint16 serverPort, QObject *parent)
    : QObject(parent),
    udpSocket(new QUdpSocket(this)),
    serverAddress(QHostAddress(serverIp)),
    serverPort(serverPort) {
  
  if (!serverAddress.isNull()) {
    qDebug() << "[QtUDPClient] Initialized for" << serverIp << ":" << serverPort;
  } else {
    emit errorOccurred("[QtUDPClient] Invalid server IP address");
  }
  
  // Connect signal for incoming datagrams
  connect(udpSocket, &QUdpSocket::readyRead, this, &QtUDPClient::handleReadyRead);
}

QtUDPClient::~QtUDPClient() {
  if (udpSocket) {
    udpSocket->close();
    qDebug() << "[QtUDPClient] Socket closed.";
  }
}

/**
 * @brief Sends an AuthRequest datagram to the server.
 */
void QtUDPClient::sendAuthRequest(std::uint16_t sessionId,
                                  const std::string& username,
                                  const std::string& password) {
  // Construct the binary payload
  AuthRequest request(sessionId, username, password);
  auto buffer = request.toBuffer();
  
  // Convert std::array to QByteArray for transmission
  QByteArray datagram(reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()));
  
  // Send datagram to the configured server address and port
  qint64 sent = udpSocket->writeDatagram(datagram, serverAddress, serverPort);
  
  if (sent < 0) {
    emit errorOccurred(QString("[QtUDPClient] Failed to send AUTH_REQUEST: %1").arg(udpSocket->errorString()));
  } else {
    qDebug() << "[QtUDPClient] Sent AUTH_REQUEST datagram (" << sent << " bytes)"
             << "to" << serverAddress.toString() << ":" << serverPort;
  }
}

/**
 * @brief Handles incoming datagrams from the server.
 *
 * This method reads each available datagram and attempts to parse it as an AuthResponse.
 * If the response is valid, it emits the authResponseReceived() signal.
 */
void QtUDPClient::handleReadyRead() {
  while (udpSocket->hasPendingDatagrams()) {
    QByteArray datagram;
    datagram.resize(static_cast<int>(udpSocket->pendingDatagramSize()));
    QHostAddress sender;
    quint16 senderPort;
    
    udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
    
    emit datagramReceived(datagram, sender, senderPort);
    
    // Attempt to parse AuthResponse (51 bytes expected)
    if (datagram.size() == static_cast<int>(AuthResponse::MESSAGE_SIZE + AuthResponse::TOKEN_SIZE + 3)) {
      try {
        std::array<uint8_t, 51> buffer{};
        std::memcpy(buffer.data(), datagram.constData(), buffer.size());
        
        // Deserialize AuthResponse
        AuthResponse response;
        response.setSessionId((buffer[0] << 8) | buffer[1]);
        response.setStatusCode(buffer[2]);
        response.setMessage(std::string(reinterpret_cast<char*>(buffer.data() + 3), AuthResponse::MESSAGE_SIZE));
        response.setSessionToken(std::string(reinterpret_cast<char*>(buffer.data() + 3 + AuthResponse::MESSAGE_SIZE), AuthResponse::TOKEN_SIZE));
        
        emit authResponseReceived(response);
        
        qDebug() << "[QtUDPClient] Received AuthResponse from"
                 << sender.toString() << ":" << senderPort
                 << "-> sessionId=" << response.getSessionId()
                 << " status=" << response.getStatusCode()
                 << " message=" << QString::fromStdString(response.getMessage())
                 << " token=" << QString::fromStdString(response.getSessionToken());
        
      } catch (const std::exception& ex) {
        emit errorOccurred(QString("[QtUDPClient] Invalid AuthResponse: %1").arg(ex.what()));
      }
    } else {
      qDebug() << "[QtUDPClient] Received datagram of unexpected size:"
               << datagram.size() << "bytes from" << sender.toString();
    }
  }
}
