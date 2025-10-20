#ifndef QTUDPCLIENT_H
#define QTUDPCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

#include "model/network/authenticationrequest.h"
#include "model/network/authenticationresponse.h"

/**
 * @class QtUDPClient
 * @brief Handles UDP transmission and reception of authentication datagrams.
 *
 * This class is responsible for sending AuthRequest packets and receiving
 * AuthResponse messages using a non-connected UDP socket.
 */
class QtUDPClient : public QObject {
  Q_OBJECT
  
public:
  explicit QtUDPClient(const QString& serverIp, quint16 serverPort, QObject* parent = nullptr);
  ~QtUDPClient();
  
  /**
   * @brief Sends an AuthRequest datagram to the server.
   * @param sessionId The session identifier.
   * @param username The username string.
   * @param password The password string.
   */
  void sendAuthRequest(std::uint16_t sessionId,
                       const std::string& username,
                       const std::string& password);
  
signals:
  /// Emitted when an AuthResponse is successfully received and parsed.
  void authResponseReceived(const AuthResponse& response);
  
  /// Emitted when a raw datagram arrives (useful for debugging).
  void datagramReceived(const QByteArray& data, const QHostAddress& sender,
    quint16 senderPort
  );
  
  /// Emitted on any socket or protocol error.
  void errorOccurred(const QString& message);
  
private slots:
  /// Handles all incoming datagrams asynchronously.
  void handleReadyRead();
  
private:
  QUdpSocket* udpSocket;
  QHostAddress serverAddress;
  quint16 serverPort;
};

#endif // QTUDPCLIENT_H
