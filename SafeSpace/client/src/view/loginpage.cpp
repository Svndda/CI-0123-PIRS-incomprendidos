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
  this->renderer = new QSvgRenderer(
      QString(":/images/bg1.svg"), this
      );
  
}

LoginPage::~LoginPage() {
  delete ui;
}

void LoginPage::on_sendCredentials_button_clicked() {
  QString email = this->ui->email_lineEdit->text();
  QString password = this->ui->password_lineEdit->text();

  qDebug() << "Usuario: " << email;
  qDebug() << "Contrasena: " << password;

  if (!(email.isEmpty() && password.isEmpty())) {
    User user(0, email.toStdString());
    user.setPassword(password.toStdString());
    this->ui->email_lineEdit->clear();
    this->ui->password_lineEdit->clear();
    
    emit this->sendCredentials(user);
  } else {
    QMessageBox::warning(this, "Error"
        , "Porfavor, rellena todos los campos solicitados.");
  } 
}



void LoginPage::on_showPassword_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        ui->password_lineEdit->setEchoMode(QLineEdit::Normal);   // Muestra la contraseña
    } else {
        ui->password_lineEdit->setEchoMode(QLineEdit::Password); // Oculta la contraseña
    }
}

