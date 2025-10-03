// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#ifndef MODEL_H
#define MODEL_H

#include <QString>
#include <QPrinter>
#include <vector>

#include "model/managers/UsersManager.h"
#include "model/filesystem/FileSystem.h"

/**
 * @class Model
 * @brief Core singleton class managing the POS system.
 *
 * Model is responsible for handling products, categories, supplies and user data.
 * It interacts with the BackupModule to load and persist data, and provides functions
 * for adding, editing, and removing items in the system.
 */
class Model {
  // Deleted copy constructor and assignment operator to prevent copying.
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;

private:
  bool started = false;       ///< Flag indicating if the model has been started.
  FileSystem filesystem;
  UsersManager usersManager;
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
  
  // /**
  //  * @brief getPageAccess Checks the user's access to the given page index.
  //  * @param page Index of the page that are going to be checked.
  //  * @return User's page acccess state.
  //  */
  // inline size_t getPageAccess(const size_t page) {
  //   // Returns the user's access for the given page index.
  //   return this->usersManager.user.getUserPermissions()[page].access;
  // }
  
  inline std::vector<User> getSystemUsers() {
    return this->usersManager.getUsers();
  }
  
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
  bool start(const User& user);
  
  /**
   * @brief Shuts down the POS model.
   *
   * Saves the current product and supply data to backup files and clears internal data.
   */
  void shutdown();
  
  bool authenticate(
    const std::string& username, const std::string& password);
  
  bool deleteUser(
    const std::string& username, const std::string& password);
  
  bool updateUser(
    const std::string& username, const User& updatedUser);
  
  bool saveUser(
    const QString &username, const QString &password, const QString &rol);
  
};

#endif // MODEL_H
