// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QApplication>
#include <vector>

// #include "model/managers/UsersManager.h"
// #include "model/filesystem/FileSystem.h"
#include "model/structures/user.h"
#include "model/network/qtudpclient.h"

struct NetworkEvent {
  QString direction;
  QString type;
  QString detail;
  QByteArray rawBytes;
  QString timestamp;
  int nodeId;  
};

struct NodeInfo {
  int id;
  QString name;
  QString ip;
  int port;
  QString status;
};

/**
 * @class Model
 * @brief Core singleton class managing the POS system.
 *
 * Model is responsible for handling products, categories, supplies and user data.
 * It interacts with the BackupModule to load and persist data, and provides functions
 * for adding, editing, and removing items in the system.
 */
class Model : public QObject {
  Q_OBJECT
  // Deleted copy constructor and assignment operator to prevent copying.
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

private:
  const std::string UNITY_PATH
      = QApplication::applicationDirPath().toStdString() + "\\unity.bin";
  bool started = false;       ///< Flag indicating if the model has been started.
  QtUDPClient client;
  User user = User("SafeAdmin", User::hashSHA256("qwerTY2134"));
  std::vector<NetworkEvent> networkLog;
  QVector<NodeInfo> nodes;  
  // FileSystem filesystem;
  // UsersManager usersManager;
private:
  /**
   * @brief Private constructor for Model.
   */
  Model();
  void addNetworkEvent(const NetworkEvent& evt);
  bool loadNodesFromFile(const QString& filename = "nodes_config.txt");
  
public:  ///< Getters
  /**
   * @brief Retrieves the singleton instance of Model.
   * @return Reference to the single instance of Model.
   */
  static Model& getInstance();
  
  const QVector<NodeInfo>& getNodes() const { return nodes; }
  NodeInfo getNodeById(int nodeId) const;
  bool updateNodeStatus(int nodeId, const QString& status);
  
  /**
   * @brief Checks if the App model has been started.
   * @return True if the model is started.
   */
  inline bool isStarted() { return this->started; }
  const std::vector<NetworkEvent>& getNetworkLog() const {
    return this->networkLog;
  };
  
  // /**
  //  * @brief getPageAccess Checks the user's access to the given page index.
  //  * @param page Index of the page that are going to be checked.
  //  * @return User's page acccess state.
  //  */
  // inline size_t getPageAccess(const size_t page) {
  //   // Returns the user's access for the given page index.
  //   return this->usersManager.user.getUserPermissions()[page].access;
  // }
  
  // inline std::vector<User> getSystemUsers() {
  //   return this->usersManager.getUsers();
  // }
  
public:  ///< Functions.
  
  /**
   * @brief Shuts down the POS model.
   *
   * Saves the current product and supply data to backup files and clears internal data.
   */
  void shutdown() {this->started = false;};
  
  
  bool authenticate(
    const std::string& username, const std::string& password);
  
  void runNode(uint8_t nodeId);
  void stopNode(uint8_t nodeId);
signals:
  bool authenticatheResponse(bool state);
  void runNodeResult(uint8_t nodeId, uint8_t status);
  void stopNodeResult(uint8_t nodeId, uint8_t status);
  void networkEventLogged(const NetworkEvent& evt);  
};

#endif // MODEL_H
