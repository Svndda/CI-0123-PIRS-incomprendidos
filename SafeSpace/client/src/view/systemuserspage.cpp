#include "systemuserspage.h"
#include "ui_systemuserspage.h"

#include "elements/rolescombobox.h"
#include "elements/button.h"
#include "colors.h"

SystemUsersPage::SystemUsersPage(QWidget *parent, Model& model) :
    Page(parent, model),
    ui(new Ui::SystemUsersPage) {
    ui->setupUi(this);
  this->refreshUsersTable();
}

SystemUsersPage::~SystemUsersPage() {
    delete ui;
}

void SystemUsersPage::refreshUsersTable() {
  
  QTableWidget* table = this->ui->userTableWidget;
  table->setRowCount(0);
  
  
  const std::vector<User> systemUsers = this->model.getSystemUsers();
  this->logger(
      LogType::DEBUG,
      "hay registrados usuarios: " + QString::number(systemUsers.size())
      );
  
  for (const auto& user : systemUsers) {
    const int row = table->rowCount();
    table->insertRow(row);
    
    QTableWidgetItem* nameField = new QTableWidgetItem(user.getUsername().data());
    table->setItem(row, 0, nameField);
    
    RolesComboBox* combo = new RolesComboBox();
    table->setCellWidget(row, 1, combo);
    
    Button* button = new Button(
        table, "Eliminar",
        Colors::LigthRed, Colors::White,
        10
        );
    
    this->connect(
        button, &QPushButton::clicked,
        this, [this, user]() {
          
          bool reply = this->askUserConfirmation(
              "Vas a eliminar un usuario."
              "\n¿Desea continuar la operación?"
          );
          
          if (reply) {
            qDebug() << "Eliminando usuario con ID: " << user.getUsername();
            
            if (this->model.deleteUser(user)) {
              std::cout << "usurio eliminado correctamente: " << user.getUsername();
              this->refreshUsersTable();
            }
          }
        });
    table->setCellWidget(row, 2, button);
  }
  
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void SystemUsersPage::usersModified() {
  this->refreshUsersTable();
}
    
