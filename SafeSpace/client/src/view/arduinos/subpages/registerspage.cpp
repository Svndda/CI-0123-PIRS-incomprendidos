#include "registerspage.h"
#include "ui_registerspage.h"

RegistersPage::RegistersPage(QWidget *parent, Model &model)
    : Page(parent, model)
    , ui(new Ui::RegistersPage) {
  ui->setupUi(this);
}

RegistersPage::~RegistersPage() {
  delete ui;
}
