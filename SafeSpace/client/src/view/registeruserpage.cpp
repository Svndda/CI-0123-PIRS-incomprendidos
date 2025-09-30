#include "registeruserpage.h"
#include "ui_registeruserpage.h"
#include <QMessageBox>

RegisterUserPage::RegisterUserPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterUserPage) {
    ui->setupUi(this);
    // Conectar el botón de registro a la señal
    connect(ui->registerButton, &QPushButton::clicked, this, [this]() {
        QString username = ui->usernameLineEdit->text();
        QString password = ui->passwordLineEdit->text();
        QString rol = ui->rolLineEdit->text();
        if(username.isEmpty() || password.isEmpty() || rol.isEmpty()){
            QMessageBox::warning(this, "Error", "Por favor, rellene todos los campos.");
            return;
        }
        emit registerRequested(username, password, rol);
        QMessageBox::information(this, "Registro exitoso", "Usuario registrado correctamente.");
        ui->usernameLineEdit->clear();
        ui->passwordLineEdit->clear();
        ui->rolLineEdit->clear();
    });
}

RegisterUserPage::~RegisterUserPage() {
    delete ui;
}

// void RegisterUserPage::on_registerButton_clicked(){
//   QString username = ui->usernameLineEdit->text();
//   QString password = ui->passwordLineEdit->text();
//   // QString type = ui->typwComboBox->currentText();

//  /* if(username.isEmpty() || password.isEmpty() || type.isEmpty()){
//       QMessageBox::warning(this, "Error", "Por favor, rellene todos los campos.");
//       return;
//   }*/
//   User user(4, username.toStdString(), "Invitado 1");
//   user.setPassword(password.toStdString());
//   this->userController->saveUser(user);

// }
