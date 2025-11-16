#ifndef QTUDPCLIENT_H
#define QTUDPCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

#include "RunNode.h"
#include "RunNodeResponse.h"
#include "StopNode.h"
#include "StopNodeResponse.h"


/**
 * @class QtUDPClient
 * @brief Handles UDP transmission and reception of authentication datagrams.
 *
 * This class is responsible for sending AuthRequest packets and receiving
 * AuthResponse messages using a non-connected UDP socket.
 */
class QtUDPClient : public QObject {
  Q_OBJECT
private:
  
  /**
   * @brief udpSocket Internal UDP socket.
   */
  QUdpSocket* udpSocket;
  
  /**
   * @brief serverAddress Target ProxyNode IP address.
   */
  QHostAddress serverAddress;
  
  /**
   * @brief serverPort Target ProxyNode UDP port
   */
  quint16 serverPort;
  
  uint16_t sessionId = 0;
  
public:
  explicit QtUDPClient(const QString& serverIp, quint16 serverPort, QObject* parent = nullptr);
  ~QtUDPClient();
  
  void sendRunNodeRequest(uint8_t nodeId);
  
  void sendStopNodeRequest(uint8_t nodeId);
  
  /**
   * @brief Sends an AuthRequest datagram to the server.
   * @param sessionId The session identifier.
   * @param username The username string.
   * @param password The password string.
   */
  void sendAuthRequest(std::uint16_t sessionId,
                       const std::string& username,
                       const std::string& password);
  
  /**
   * @brief Sends a ConnectRequest to subscribe this client to sensor broadcasts.
   * @param sessionId Unique session ID used for subscription.
   */
  void sendConnectRequest(std::uint16_t sessionId);
  
signals:
  // /**
  //  * @brief Emitted when an AuthResponse is successfully received and parsed.
  // */
  // void authResponseReceived(const AuthResponse& response);
  
  /**
   * @brief Emitted when a raw datagram arrives (useful for debugging).
  */  
  void datagramReceived(const QByteArray& data, const QHostAddress& sender,
    quint16 senderPort
  );
  
  // Emitted when a RunNodeResponse is received and parsed correctly
  void runNodeResponseReceived(const RunNodeResponse& response);
  
  // Emitted when a StopNodeResponse is received and parsed correctly
  void stopNodeResponseReceived(const StopNodeResponse& response);
  
  /**
   * @brief Emitted on any socket or protocol error.
  */
  void errorOccurred(const QString& message);
  
private slots:
  /**
   * @brief Handles all pending UDP datagrams and routes them to the appropriate handlers.
   *
   * This slot is triggered automatically whenever the underlying QUdpSocket
   * receives new data. It reads each datagram, determines its size and type,
   * and delegates its processing to specialized handler functions:
   */
  void handleReadyRead();
  
private:
 // /**
 // * @brief Processes an AuthResponse datagram received from the Proxy or AuthServer.
 // *
 // * @param datagram The raw datagram payload containing the AuthResponse.
 // * @param sender The IP address of the sender (Proxy or AuthServer).
 // * @param senderPort The UDP port of the sender.
 // *
 // * @throws std::runtime_error if the datagram is invalid or truncated.
 // *
 // * @see AuthResponse, sendConnectRequest(), authResponseReceived()
 // */
 //  void handleAuthResponseDatagram(const QByteArray &datagram,
 //                                  const QHostAddress &sender,
 //                                  quint16 senderPort);  
  
  /**
 * @brief Handles unexpected or unrecognized UDP datagrams.
 *
 * @param datagram Raw datagram data that could not be classified.
 * @param sender IP address of the sender.
 * @param senderPort UDP port number of the sender.
 *
 * @see handleReadyRead()
 */
  void handleUnknownDatagram(
      const QByteArray &datagram,
      const QHostAddress &sender,
      quint16 senderPort
  );
  
};

#endif // QTUDPCLIENT_H
