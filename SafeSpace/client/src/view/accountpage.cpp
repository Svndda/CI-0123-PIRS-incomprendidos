#include "accountpage.h"
#include "src/view/ui_accountpage.h"
#include "ui_accountpage.h"

AccountPage::AccountPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::AccountPage) {
  ui->setupUi(this);
}

AccountPage::~AccountPage() {
  delete ui;
}
