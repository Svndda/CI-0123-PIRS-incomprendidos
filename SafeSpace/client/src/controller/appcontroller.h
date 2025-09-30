// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QMainWindow>
#include <QStackedWidget>
#include "model/model.h"
#include "model/FileSystem.h"
#include "UserController.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @class AppController
 * @brief Main application controller managing UI and workflow.
 *
 * This class coordinates the application pages, user interactions,
 * and communication with the model. It manages page transitions,
 * enables/disables UI controls, and processes user authentication.
 */
class AppController : public QMainWindow {
  Q_OBJECT

public:
  /**
   * @brief Constructs the AppController.
   * @param parent Pointer to the parent widget.
   */
  explicit AppController(QWidget* parent = nullptr);
  
  /**
   * @brief Destroys the AppController and performs cleanup.
   */
  ~AppController();
  
   /**
   * @brief Gets a reference to the file system.
   * @return Reference to the file system.
   */
  FileSystem& getFileSystem();

private:
  Ui::MainWindow* ui;                      ///< Pointer to the main UI layout.
  QStackedWidget* pageStack;               ///< Stack widget managing application pages.
  Model& model = Model::getInstance(); ///< Reference to the singleton model.
  FileSystem filesystem = FileSystem();
  UserController usercontroller = UserController(&filesystem);

private:
  /**
   * @brief Sets up the connections between UI signals and controller slots.
   */
  void setupConnections();
  
  /**
   * @brief Sets all navigation buttons of the main layout to the given state.
   */
  void setButtonsState(bool state);
  
  /**
   * @brief Prepares and creates system pages.
   */
  void prepareSystemPages();
  
  /**
   * @brief Refreshes the page stack based on the given index.
   * 
   * This function checks if the application model is started and retrieves 
   * the current user's permissions. If the user has access to the requested 
   * page, it switches to the corresponding page.
   * 
   * @param stackIndex The index of the page stack to switch to.
   */
  void refreshPageStack(const size_t pageIndex);
  
  /**
   * @brief Switches the displayed page in the stack widget.
   * @param pageIndex The index of the page to display.
   */
  void switchPages(const size_t pageIndex);
  
private slots:
  
  /**
   * @brief Processes the user authentication.
   */
  void authenticate(const std::string& username, const std::string& password);
  
  /**
   * @brief Deletes the given user.
   */
  void deleteUser(const std::string& username, const std::string& password);
  
  /**
   * @brief Updates the given specified user withe the new one.
   */
  void updateUser(const std::string& username, const User& updatedUser);
  
  /**
   * @brief Saves the given user information.
   */
  void saveUser(const QString &username, const QString &password, const QString &rol);
  
  /**
   * @brief Resets the application state to its initial configuration.
   * 
   * This function shuts down the model and resets the page stack to its 
   * initial state by setting the current index to 0, which corresponds
   * to the login page.
   */
  void resetApplicationState();

 
};
#endif // APPCONTROLLER_H
