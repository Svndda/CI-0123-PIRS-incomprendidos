#include "administrationpage.h"
#include "ui_administrationpage.h"
#include "systemuserspage.h"
#include "registrationpage.h"

AdministrationPage::AdministrationPage(
    QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::AdministrationPage)
    , pageStack(new QStackedWidget(this)) {
  ui->setupUi(this);
  
  this->ui->pages_layout->addWidget(this->pageStack);
  SystemUsersPage* systemUsersPage = new SystemUsersPage(this->pageStack, this->model);
  RegistrationPage* registrationPage = new RegistrationPage(this->pageStack, this->model);\
  
  this->pageStack->addWidget(systemUsersPage);
  this->pageStack->addWidget(registrationPage);
  
  connect(this->ui->system_users_button, &QPushButton::clicked, this, [this]() {
    this->pageStack->setCurrentIndex(0);
    qDebug() << "Botón clicado!";
  });
  
  connect(this->ui->register_user_button, &QPushButton::clicked, this, [this]() {
    this->pageStack->setCurrentIndex(1);
    qDebug() << "Botón clicado!";
  });
}

AdministrationPage::~AdministrationPage() {
  delete ui;
}

void AdministrationPage::registerUser(
    const QString &username, const QString &password, const QString &rol) {
  emit this->saveUser(username, password, rol);
}
