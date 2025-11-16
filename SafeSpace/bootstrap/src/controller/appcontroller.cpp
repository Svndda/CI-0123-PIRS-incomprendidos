// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include "appcontroller.h"
#include <iostream>

#include "ui_mainwindow.h"

#include <QMessageBox>

#include "model.h"
#include "user.h"
#include "loginpage.h"
#include "nodes/nodespage.h"

AppController::AppController(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , pageStack(new QStackedWidget(this))
  , model(Model::getInstance()) {
  // Define the controller ui as the mainWindow.
  ui->setupUi(this);
  // Connects all the ui elements to their slot functions.
  this->setupConnections();
  // For the app to be in full window.
  this->showMaximized();
}

AppController::~AppController() {
  // Deletes the page stack.
  delete this->pageStack;
  
  // Delete the ui.
  delete this->ui;
}

void AppController::setupConnections() {
  // Adds the pages stack in his corresponding area of the program ui.
  this->ui->mainLayout->addWidget(this->pageStack, 2, 0);
  // Creates the pos loggin page to manage the user's loggin.
  LoginPage* loginPage = new LoginPage(this, this->model);
  // Add the login page to the stack view.
  this->pageStack->addWidget(loginPage);
  this->pageStack->setCurrentIndex(0);
  // this->filesystem.readFile("UserList");
  // Connects the login signal to the controller function to try
  //  start the system.
  this->connect(
    loginPage, &LoginPage::userAuthenticated,
    this, &AppController::userAuthenticated
  );  
  // this->userAuthenticated();
}

// void AppController::setButtonsState(bool state) {
//   // Enables all the system pages buttons.
//   this->ui->arduinosPage_button->setVisible(state);
//   this->ui->nodesPage_button->setVisible(state);
//   this->ui->administrationPage_button->setVisible(state);
//   this->ui->accountPage_button->setVisible(state);
// }

void AppController::prepareSystemPages() {
  if (this->pageStack->count() == 1) {
    // Creates the different program pages.
    NodesPage* nodesPage = new NodesPage(this, this->model);
    
    this->connect(
        nodesPage, &NodesPage::logout,
        this, &AppController::resetApplicationState
        );
    
    // // Adds the program pages to the stack of pages.
    this->pageStack->addWidget(nodesPage);
  }
  
  // // Sets the stack page to the nodes page.
  this->switchPages(1);
}

void AppController::switchPages(const size_t stackIndex) {
  // Checks if the model is started.
  // Checks if the user has allowed access to the clicked button's page.
  if (true /*!= User::PageAccess::DENIED*/) {
    // Switch the page.
    // Sets the indexed page of the stack to the requested one.
    qDebug() << this->pageStack->indexOf(this->pageStack->currentWidget());
    this->pageStack->setCurrentIndex(stackIndex);
    qDebug() << this->pageStack->indexOf(this->pageStack->currentWidget());
  } 
}

// void AppController::pageButtonsRefresh(const size_t pageIndex) {  
  
//   // Vector of the application buttons to move through the pages.
//   QVector<QPushButton*> buttons = {
//     this->ui->nodesPage_button,
//     this->ui->arduinosPage_button,
//     this->ui->administrationPage_button,
//     this->ui->accountPage_button
//   };
//   // Iterate over the buttons and widgets to update their states
//   for (size_t i = 0; i < buttons.size(); ++i) {
//     // Boolean that indicates if the current index is the requested page.
//     buttons[i]->setChecked(i == pageIndex);
//   }
// }

void AppController::userAuthenticated() {
  this->prepareSystemPages();
  // this->setButtonsState(true);
  qDebug() << "credenciales aceptadas";
}

void AppController::resetApplicationState() {
  this->pageStack->setCurrentIndex(0);
}
