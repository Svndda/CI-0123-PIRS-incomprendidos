// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include "appcontroller.h"
#include "ui_mainwindow.h"

#include <QMessageBox>

#include "model/model.h"
#include "model/user.h"
#include "view/loginpage.h"
#include "view/administrationpage.h"
#include "colors.h"

AppController::AppController(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , pageStack(new QStackedWidget(this))
  , model(Model::getInstance()) {
  // Define the controller ui as the mainWindow.
  ui->setupUi(this);
  this->ui->page1->setButtonText("Sensores");
  this->ui->page2->setButtonText("Reportes");
  this->ui->page3->setButtonText("Administración");
  // Connects all the ui elements to their slot functions.
  this->setupConnections();
  this->setStyleSheet("background-color: " + Colors::Black + ";");
  
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
  this->ui->mainLayout->addWidget(this->pageStack, 1, 0);
  // Creates the pos loggin page to manage the user's loggin.
  LoginPage* loginPage = new LoginPage(this, this->model);
  // Add the login page to the stack view.
  this->pageStack->addWidget(loginPage);
  this->pageStack->setCurrentIndex(0);
  // Connects the login signal to the controller function to try
  //  start the system.
  this->connect(
    loginPage, &LoginPage::sendCredentials,
    this, &AppController::userAccepted
  );
  // Hides/Disables the pages buttons.
  this->setButtonsState(false);
}

void AppController::setButtonsState(bool state) {
  // Enables all the system pages buttons.
  this->ui->page1->setVisible(state);
  this->ui->page2->setVisible(state);
  this->ui->page3->setVisible(state);
}

void AppController::prepareSystemPages() {
  // // Creates the different program pages.
  AdministrationPage* admPage = new AdministrationPage(this, this->model);
  // Inventory* inventoryPage = new Inventory(this, this->model);
  // Users* usersPage = new Users(this, this->model);
  // Settings* settingsPage = new Settings(this, this->model);
  
  // // Adds the program pages to the stack of pages.
  this->pageStack->addWidget(admPage);
  // this->pageStack->addWidget(inventoryPage);
  // this->pageStack->addWidget(usersPage);
  // this->pageStack->addWidget(settingsPage);
  
  // this->connect(settingsPage, &Settings::logoutCurrentUser
  //     , this, &AppController::resetApplicationState);
  
  // // Sets the stack page to the pos.
  this->refreshPageStack(1);
}

void AppController::refreshPageStack(const size_t stackIndex) {
  // Checks if the model is started.
  if (/*this->model.isStarted()*/ true) {
    // Checks if the user has allowed access to the clicked button's page.
    if (true /*!= User::PageAccess::DENIED*/) {
      // Switch the page.
      qDebug() << "refrescando pagina";
      this->switchPages(stackIndex);
    } 
  }
}

void AppController::switchPages(const size_t pageIndex) {
  // Sets the indexed page of the stack to the requested one.
  qDebug() << this->pageStack->indexOf(this->pageStack->currentWidget());  
  this->pageStack->setCurrentIndex(pageIndex);
  qDebug() << this->pageStack->indexOf(this->pageStack->currentWidget());
  
  // Buttons offset.
  const size_t buttonsOffset = pageIndex - 1;
  
  // Vector of the application buttons to move through the pages.
  QVector<QPushButton*> buttons = {
    this->ui->page1,
    this->ui->page2,
    this->ui->page3
  };
  
  // Vector of the rectangle widgets that are sync with the current button page.
  QVector<QWidget*> widgets = {
  };
  
  // Iterate over the buttons and widgets to update their states
  for (size_t i = 0; i < buttons.size(); ++i) {
    // Boolean that indicates if the current index is the requested page.
    const bool isSelected = (i == buttonsOffset);
    buttons[i]->setChecked(isSelected);
    // // Use a conditional to set the style sheet: green for selected,
    // // transparent otherwise.
    // widgets[i]->setStyleSheet(
    //     QString("QWidget { background-color: %1; }")
    //         .arg(isSelected ? "rgb(0, 153, 73)" : "transparent")
    //     );
  }
}


void AppController::userAccepted(const User user) {
  // Start, tell the model to prepare his information.  
  if (/*this->model.start(user)*/ 1) {
    // Creates 
    this->prepareSystemPages();
    // Enables the page buttons.
    this->setButtonsState(true);
    qDebug() << "credenciales aceptadas";
  } else {
    QMessageBox::information(this, "Credenciales erroneas"
        , "Por favor, verifique los datos ingresados.");
  }
}

void AppController::resetApplicationState() {
  this->pageStack->setCurrentIndex(0);
}
