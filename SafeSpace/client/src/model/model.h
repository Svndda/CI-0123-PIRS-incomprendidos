// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QApplication>
#include <QPrinter>
#include <vector>

// #include "model/managers/UsersManager.h"
// #include "model/filesystem/FileSystem.h"
#include "model/structures/user.h"
#include "model/network/qtudpclient.h"
#include "model/structures/sensordata.h"

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
  uint16_t sessionId;
  std::vector<User> systemUsers;  
  Token16 token;
  User user = User();
  std::vector<SensorData> sensorsData;
  
  // FileSystem filesystem;
  // UsersManager usersManager;
private:
  /**
   * @brief Private constructor for Model.
   */
  Model();
  
public:  ///< Getters
  /**
   * @brief Retrieves the singleton instance of Model.
   * @return Reference to the single instance of Model.
   */
  static Model& getInstance();
  
  /**
   * @brief Checks if the App model has been started.
   * @return True if the model is started.
   */
  inline bool isStarted() { return this->started; }

  inline std::vector<SensorData>& getSensorsData() {return this->sensorsData;}
  
  inline void sendSystemUsersRequest() {this->client.sendGetSystemUsersRequest(this->sessionId);}
  
  inline User getUser() {return this->user;}
  
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
   * @brief Starts the POS model.
   *
   * Loads user data from backup and, if the provided user is registered, loads
   * product and supply data into memory.
   *
   * @param user The user to start the model with.
   * @return True if the model has been successfully started.
   */
  bool start(/*const User& user*/);
  
  /**
   * @brief Fully resets the internal state of the application model.
   *
   * This method clears:
   *  - Authentication session
   *  - Cached users
   *  - Sensor data buffers
   *  - Network tokens and session IDs
   *  - Internal started flag
   *
   * After calling this method, the model returns to a clean
   * pre-authentication state.
   */
  void reset();
  
  /**
   * @brief Shuts down the POS model.
   *
   * Saves the current product and supply data to backup files and clears internal data.
   */
  void shutdown();
  
  void authenticate(
    const std::string& username, const std::string& password);
  
  bool deleteUser(
    const std::string& username);
  
  bool deleteUser(
      const User& user);
  
  bool updateUser(
    const std::string& username, const User& updatedUser);
  
  bool saveUser(
    const QString &username, const QString &password, const QString &rol);
  
  inline const std::vector<User>& getSystemUsers() const {
    return systemUsers;
  }
  
  // User findUser(const std::string& username) {
  //   return this->usersManager.findUser(username);
  // };
  
private slots:
  void onSystemUsersResponseReceived(const GetSystemUsersResponse& response);
  
signals:
  bool authenticatheResponse(bool state);
  SensorData sensorDataReceived(const SensorData);
  void systemUsersReceived(const std::vector<User>& users);  
};

#endif // MODEL_H
