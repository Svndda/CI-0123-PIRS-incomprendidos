#include "registeruserpage.h"
#include "ui_registeruserpage.h"

RegisterUserPage::RegisterUserPage(UserController* userController, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterUserPage),
    userController(userController)
{
    ui->setupUi(this);
    // Conectar el botón de registro a la señal
    connect(ui->registerButton, &QPushButton::clicked, this, [this]() {
        QString username = ui->usernameLineEdit->text();
        QString password = ui->passwordLineEdit->text();
        QString rol = ui->rolLineEdit->text();
        emit registerRequested(username, password, rol);
    });

}

RegisterUserPage::~RegisterUserPage() {
    delete ui;
}

void RegisterUserPage::on_registerButton_clicked(){
  Qstring username = ui->usernameLineEdit->text();
  Qstring password = ui->passwordLineEdit->text();
  Qstring type = ui->typwComboBox->currentText();

  if(username.isEmpty() || password.isEmpty() || type.isEmpty()){
      QMessageBox::warning(this, "Error", "Por favor, rellene todos los campos.");
      return;
  }

  userController->registerUser(username, password, type);
}