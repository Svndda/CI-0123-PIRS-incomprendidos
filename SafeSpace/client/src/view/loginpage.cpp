#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "loginpage.h"
#include "ui_loginpage.h"

LoginPage::LoginPage(QWidget *parent,
  Model& model) :
    Page(parent, model)
    , ui(new Ui::LoginPage) {
  ui->setupUi(this);
    ui->password_lineEdit->setEchoMode(QLineEdit::Password); // Oculta la contraseña
  connect(this->ui->sendCredentials_button, &QPushButton::clicked, this, []() {
    qDebug() << "Botón clicado!";
  });
  // this->renderer = new QSvgRenderer(
  //   QString(":/images/bg1.svg"), this
  // );
}

LoginPage::~LoginPage() {
  delete ui;
}

void LoginPage::on_sendCredentials_button_clicked() {
  std::string username = this->ui->email_lineEdit->text().toStdString();
  std::string password = this->ui->password_lineEdit->text().toStdString();
  
  if (username.empty() | password.empty()) {
    this->warningMessageBox("Por favor, rellene todos los campos.");    
  }

  if (this->model.authenticate(username, password)) {    
    emit this->userAuthenticated();
  } else {
    this->warningMessageBox("Credenciales de cuenta erróneas.");
  }
}

void LoginPage::on_showPassword_checkBox_checkStateChanged(
    const Qt::CheckState &arg1) {
    if (arg1 == Qt::Checked) {
      this->ui->password_lineEdit->setEchoMode(QLineEdit::Normal);
    } else {
      // Hides the password.
      this->ui->password_lineEdit->setEchoMode(QLineEdit::Password);
    }
}

