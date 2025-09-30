#include "administrationpage.h"
#include "ui_administrationpage.h"

AdministrationPage::AdministrationPage(
    QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::AdministrationPage) {
  ui->setupUi(this);
  connect(this->ui->system_users_button, &QPushButton::clicked, this, [this]() {
    this->ui->pagesStack->setCurrentWidget(this->ui->usersList);
    qDebug() << "Botón clicado!";
  });
  
  connect(this->ui->register_user_button, &QPushButton::clicked, this, [this]() {
    this->ui->pagesStack->setCurrentWidget(this->ui->registerUserForm);
    qDebug() << "Botón clicado!";
  });
  
  this->connect(
    this->ui->registerUserForm, &RegisterUserPage::registerRequested,
    this, &AdministrationPage::registerUser
  );
}

AdministrationPage::~AdministrationPage() {
  delete ui;
}

void AdministrationPage::registerUser(
    const QString &username, const QString &password, const QString &rol) {
  emit this->saveUser(username, password, rol);
}
