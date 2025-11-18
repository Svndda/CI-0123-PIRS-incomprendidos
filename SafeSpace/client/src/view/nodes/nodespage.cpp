#include "nodespage.h"
#include "model.h"
#include "ui_nodespage.h"

NodesPage::NodesPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::NodesPage) {
  ui->setupUi(this);
}

NodesPage::~NodesPage()
{
  delete ui;
}
