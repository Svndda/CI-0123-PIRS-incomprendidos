#include "visualizerspage.h"
#include "ui_visualizerspage.h"

VisualizersPage::VisualizersPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::VisualizersPage) {
  ui->setupUi(this);
}

VisualizersPage::~VisualizersPage() {
  delete ui;
}
