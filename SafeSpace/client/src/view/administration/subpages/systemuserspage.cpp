#include "systemuserspage.h"
#include "ui_systemuserspage.h"

#include "elements/rolescombobox.h"
#include "elements/button.h"
#include "colors.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <iostream>

SystemUsersPage::SystemUsersPage(QWidget *parent, Model& model)
    : Page(parent, model),
    ui(new Ui::SystemUsersPage) {
  
  ui->setupUi(this);
  
  // Configurar la tabla
  ui->userTableWidget->setColumnCount(3);
  ui->userTableWidget->setHorizontalHeaderLabels(
      QStringList() << "Usuario" << "Grupo/Rol" << "Acciones");
  ui->userTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui->userTableWidget->verticalHeader()->setVisible(false);
  
  // Conectar la señal del modelo
  QObject::connect(
      &this->model,
      &Model::systemUsersReceived,
      this,
      &SystemUsersPage::onSystemUsersReceived
      );
  
  // Solicitar datos inmediatamente si el modelo ya está autenticado
  if (model.isStarted()) {
    this->requestUsersFromServer();
  }
}

SystemUsersPage::~SystemUsersPage() {
  delete ui;
}

void SystemUsersPage::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  
  qInfo() << "SystemUsersPage shown - requesting users";
  
  // Forzar actualización cada vez que se muestra la página
  this->requestUsersFromServer();
}

void SystemUsersPage::hideEvent(QHideEvent *event) {
  QWidget::hideEvent(event);
  
  qInfo() << "SystemUsersPage hidden";
  // Opcional: limpiar la tabla al ocultar
  // ui->userTableWidget->setRowCount(0);
}

void SystemUsersPage::requestUsersFromServer() {
  if (model.isStarted()) {
    qInfo() << "Requesting system users from server...";
    
    // Mostrar mensaje de carga
    ui->userTableWidget->setRowCount(0);
    ui->userTableWidget->setColumnCount(1);
    ui->userTableWidget->setHorizontalHeaderLabels(QStringList() << "Estado");
    
    QTableWidgetItem* loadingItem = new QTableWidgetItem("Cargando usuarios...");
    loadingItem->setTextAlignment(Qt::AlignCenter);
    loadingItem->setForeground(QBrush(Qt::blue));
    
    ui->userTableWidget->insertRow(0);
    ui->userTableWidget->setItem(0, 0, loadingItem);
    ui->userTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // Solicitar usuarios al servidor
    model.sendSystemUsersRequest();
    
    // También solicitar datos locales (por si acaso)
    QTimer::singleShot(100, this, [this]() {
      this->onSystemUsersReceived(this->model.getSystemUsers());
    });
    
  } else {
    qWarning() << "Cannot request users: Model not authenticated";
    
    // Mostrar mensaje de error en la tabla
    ui->userTableWidget->setRowCount(0);
    ui->userTableWidget->setColumnCount(1);
    ui->userTableWidget->setHorizontalHeaderLabels(QStringList() << "Estado");
    
    QTableWidgetItem* errorItem = new QTableWidgetItem("No autenticado. Por favor inicie sesión.");
    errorItem->setTextAlignment(Qt::AlignCenter);
    errorItem->setForeground(QBrush(Qt::red));
    
    ui->userTableWidget->insertRow(0);
    ui->userTableWidget->setItem(0, 0, errorItem);
    ui->userTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    QMessageBox::warning(this, "No autenticado", 
                         "Debe autenticarse primero para ver los usuarios del sistema.");
  }
}

void SystemUsersPage::onSystemUsersReceived(const std::vector<User>& systemUsers) {
  qInfo() << "Drawing users - count:" << systemUsers.size();
  
  QTableWidget* table = this->ui->userTableWidget;
  
  // Restaurar columnas originales
  table->setColumnCount(3);
  table->setHorizontalHeaderLabels(
      QStringList() << "Usuario" << "Grupo/Rol" << "Acciones");
  
  table->setRowCount(0);
  
  if (systemUsers.empty()) {
    // Mostrar mensaje si no hay usuarios
    table->insertRow(0);
    
    QTableWidgetItem* emptyItem = new QTableWidgetItem("No hay usuarios en el sistema");
    emptyItem->setTextAlignment(Qt::AlignCenter);
    emptyItem->setForeground(QBrush(Qt::gray));
    
    table->setItem(0, 0, emptyItem);
    table->setSpan(0, 0, 1, 3);  // Combinar celdas
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    qInfo() << "No users to display";
    return;
  }
  
  for (const auto& user : systemUsers) {
    const int row = table->rowCount();
    table->insertRow(row);
    
    // Username
    QTableWidgetItem* nameField = new QTableWidgetItem(user.getUsername().c_str());
    nameField->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, nameField);
    
    // Role selector
    RolesComboBox* combo = new RolesComboBox();
    QString currentGroup = QString::fromStdString(user.getGroup());
    combo->setCurrentText(currentGroup.isEmpty() ? "Usuario" : currentGroup);
    table->setCellWidget(row, 1, combo);
    
    // Delete button
    Button* button = new Button(
        table,
        "Eliminar",
        Colors::LigthRed,
        Colors::White,
        10
        );
    
    QObject::connect(
        button,
        &QPushButton::clicked,
        this,
        [this, user]() {
          bool reply = this->askUserConfirmation(
              QString("¿Está seguro que desea eliminar al usuario '%1'?")
                  .arg(QString::fromStdString(user.getUsername()))
              );
          
          if (reply) {
            qInfo() << "Deleting user:" << QString::fromStdString(user.getUsername());
            if (this->model.deleteUser(user)) {
              this->logger(LogType::INFO, "User deleted successfully");
              
              // Refrescar la tabla después de eliminar
              QTimer::singleShot(500, this, [this]() {
                this->requestUsersFromServer();
              });
            } else {
              QMessageBox::warning(this, "Error", 
                                   "No se pudo eliminar el usuario.");
            }
          }
        }
        );
    
    table->setCellWidget(row, 2, button);
  }
  
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  qInfo() << "Table updated with" << systemUsers.size() << "users";
}
