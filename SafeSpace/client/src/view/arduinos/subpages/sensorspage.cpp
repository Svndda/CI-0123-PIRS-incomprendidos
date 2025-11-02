#include "sensorspage.h"
#include "ui_sensorspage.h"

SensorsPage::SensorsPage(QWidget *parent, Model& model) :
    Page(parent, model)
    , ui(new Ui::SensorsPage) {
  ui->setupUi(this);
  
  auto sensorsData = this->model.getSensorsData();
  
  // QTableWidget* table = this->ui->arduinos_table;
  // table->setRowCount(0);
  // const int row = table->rowCount();
  this->drawSensorsTable();
}

SensorsPage::~SensorsPage() {
  delete ui;
}

void SensorsPage::drawSensorsTable() {
  QTableWidget* table = this->ui->sensors_table;
  table->setRowCount(0);
  
  for(const auto& sensor : this->model.getSensorsData()) {
    const int row = table->rowCount();
    table->insertRow(row);
    
    QVector<QTableWidgetItem*> rowFields = this->sensorDeserialization(sensor);
    for (size_t i = 0; i < rowFields.size(); ++i) {
      table->setItem(row, i, rowFields[i]);
    }
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
  
  auto deserializeToItem = [](int value) {
    auto item = new QTableWidgetItem(QString::number(value));
    item->setTextAlignment(Qt::AlignCenter);
    return item;
  };

  qInfo() << sensor.id << sensor.timestamp << sensor.distance;

  fields.emplace_back(deserializeToItem(sensor.id));
  fields.emplace_back(deserializeToItem(sensor.timestamp));
  fields.emplace_back(deserializeToItem(sensor.distance));
  fields.emplace_back(deserializeToItem(sensor.movement));
  fields.emplace_back(deserializeToItem(sensor.temperature));
  fields.emplace_back(deserializeToItem(sensor.uv));
  fields.emplace_back(deserializeToItem(sensor.microphone));
  fields.emplace_back(deserializeToItem(sensor.led));
  fields.emplace_back(deserializeToItem(sensor.buzzer));
  fields.emplace_back(deserializeToItem(sensor.ligth));

  return fields;
}
