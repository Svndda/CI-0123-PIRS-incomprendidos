#include "registrationpage.h"
#include "ui_registrationpage.h"
#include <QMessageBox>

RegistrationPage::RegistrationPage(QWidget *parent, Model& model) :
  Page(parent, model),
  ui(new Ui::RegistrationPage) {
  ui->setupUi(this);
  // Conectar el botón de registro a la señal
  connect(ui->registerButton, &QPushButton::clicked, this, [this]() {
    QString username = ui->usernameLineEdit->text();
    QString password = ui->passwordLineEdit->text();
    QString rol = ui->rolesComboBox->currentText();
    if(username.isEmpty() || password.isEmpty() || rol.isEmpty()){
      QMessageBox::warning(this, "Error", "Por favor, rellene todos los campos.");
      return;
    }
    
    if (this->model.saveUser(username, password, rol)) {
      QMessageBox::information(this, "Registro exitoso", "Usuario registrado correctamente.");
      ui->usernameLineEdit->clear();
      ui->passwordLineEdit->clear();
      ui->rolesComboBox->setCurrentIndex(0);
    }
  });
}

RegistrationPage::~RegistrationPage() {
    delete ui;
}

