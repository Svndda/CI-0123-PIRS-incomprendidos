#include "arduinospage.h"
#include "registerspage.h"
#include "ui_arduinospage.h"
#include "sensorspage.h"
#include "machinespage.h"

ArduinosPage::ArduinosPage(QWidget *parent, Model& model)
    : Page(parent, model)
    , ui(new Ui::ArduinosPage)
    , pageStack(new QStackedWidget(this)){
  ui->setupUi(this);
  
  this->ui->subpages_layout->addWidget(this->pageStack);
  SensorsPage* sensorsPage = new SensorsPage(this->pageStack, this->model);
  MachinesPage* machinesPage = new MachinesPage(this->pageStack, this->model);
  RegistersPage* registersPage = new RegistersPage(this->pageStack, this->model);
  
  this->pageStack->addWidget(sensorsPage);
  this->pageStack->addWidget(machinesPage);
  this->pageStack->addWidget(registersPage);  
  
  connect(this->ui->sensors_button, &QPushButton::clicked, this, [this]() {
    this->pageStack->setCurrentIndex(0);
    qDebug() << "Botón clicado!";
  });
  
  connect(this->ui->machines_button, &QPushButton::clicked, this, [this]() {
    this->pageStack->setCurrentIndex(1);
    qDebug() << "Botón clicado!";
  });
  
  connect(this->ui->registers_button, &QPushButton::clicked, this, [this]() {
    this->pageStack->setCurrentIndex(2);
    qDebug() << "Botón clicado!";
  });
}

ArduinosPage::~ArduinosPage() {
  delete ui;
}
