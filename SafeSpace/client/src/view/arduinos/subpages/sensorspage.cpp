#include "sensorspage.h"
#include "ui_sensorspage.h"

#include <QDateTime>

SensorsPage::SensorsPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::SensorsPage) {
  ui->setupUi(this);
  
  QTableWidget* table = this->ui->sensors_table;
  table->setRowCount(0);
  
  
  // auto sensorsData = this->model.getSensorsData();
  
  // QTableWidget* table = this->ui->arduinos_table;
  // table->setRowCount(0);
  // const int row = table->rowCount();
  this->connect(&this->model, &Model::sensorDataReceived,
                this, [this](const SensorData& data) {
    qInfo() << "Obtenido sensor" << data.distance;
    this->drawSensorsRow(data);
  });
}

SensorsPage::~SensorsPage() {
  delete ui;
}

void SensorsPage::drawSensorsRow(const SensorData& sensor) {
  QTableWidget* table = this->ui->sensors_table;  
  const int row = table->rowCount();
  table->insertRow(row);
  QVector<QTableWidgetItem*> rowFields = this->sensorDeserialization(sensor);
  
  for (size_t i = 0; i < rowFields.size(); ++i) {
    table->setItem(row, i, rowFields[i]);
  }
  
  // Resize columns to fit their content first
  table->resizeColumnsToContents();
  
  // Then make columns stretch to fill all available width
  table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  
  // // Optional: Adjust row height automatically
  // table->resizeRowsToContents();
  
  // Optional: Disable word wrapping for cleaner layout
  table->setWordWrap(false);
}

QVector<QTableWidgetItem*> SensorsPage::sensorDeserialization(
    const SensorData& sensor) {
  
  QVector<QTableWidgetItem*> fields;
  
  auto deserializeToItem = [](float value) {
    auto item = new QTableWidgetItem(QString::number(value));
    item->setTextAlignment(Qt::AlignCenter);
    return item;
  };

  qInfo() << "deserializando sensor" << sensor.distance;
  
  QDateTime now = QDateTime::currentDateTime();
  auto timeItem = new QTableWidgetItem(now.toString("yyyy-MM-dd HH:mm:ss"));
  timeItem->setTextAlignment(Qt::AlignCenter);
  fields.emplace_back(timeItem);
  fields.emplace_back(deserializeToItem(sensor.distance));
  fields.emplace_back(deserializeToItem(sensor.temperature));
  fields.emplace_back(deserializeToItem(sensor.pressure));
  fields.emplace_back(deserializeToItem(sensor.sealevelPressure));
  fields.emplace_back(deserializeToItem(sensor.altitude));
  fields.emplace_back(deserializeToItem(sensor.realAltitude));

  return fields;
}
