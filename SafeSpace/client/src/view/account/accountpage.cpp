#include "accountpage.h"
#include "src/view/ui_accountpage.h"
#include "ui_accountpage.h"

AccountPage::AccountPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::AccountPage) {
  ui->setupUi(this);
  this->connect(this->ui->logout_button, &QPushButton::clicked,
      this, [this]() {
    if (this->askUserConfirmation("¿Cerrar sesión?")) {
      emit this->logout_requested();
    }
  });
}

AccountPage::~AccountPage() {
  delete ui;
}
