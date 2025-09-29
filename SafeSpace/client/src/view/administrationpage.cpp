#include "administrationpage.h"
#include "ui_administrationpage.h"


AdministrationPage::AdministrationPage(
    QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::AdministrationPage) {
  ui->setupUi(this);
}

AdministrationPage::~AdministrationPage() {
  delete ui;
}
