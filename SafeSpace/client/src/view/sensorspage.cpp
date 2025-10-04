#include "sensorspage.h"
#include "ui_sensorspage.h"

SensorsPage::SensorsPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::SensorsPage) {
  ui->setupUi(this);
}

SensorsPage::~SensorsPage()
{
  delete ui;
}
