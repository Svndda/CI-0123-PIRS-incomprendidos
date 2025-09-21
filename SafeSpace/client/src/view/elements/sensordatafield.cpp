#include "sensordatafield.h"
#include "ui_sensordatafield.h"

SensorDataField::SensorDataField(QWidget *parent, SensorData data)
    : QWidget(parent), sensorData(data)
    , ui(new Ui::SensorDataField) {
  ui->setupUi(this);
  this->ui->distance_label->setText(QString::number(this->sensorData.distance));
  this->ui->movement_label->setText(QString::number(this->sensorData.movement));
  this->ui->temperature_label->setText(QString::number(this->sensorData.temperature));
  this->ui->uv_label->setText(QString::number(this->sensorData.uv));
  this->ui->microphone_label->setText(QString::number(this->sensorData.microphone));
  this->ui->led_label->setText(QString::number(this->sensorData.led));
  this->ui->buzzer_label->setText(QString::number(this->sensorData.buzzer));
  this->ui->ligth_label->setText(QString::number(this->sensorData.ligth));
}

SensorDataField::~SensorDataField() {
  delete ui;
}
