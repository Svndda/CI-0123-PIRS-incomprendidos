#include "registerspage.h"
#include "ui_registerspage.h"

#include <QDateTime>

RegistersPage::RegistersPage(QWidget *parent, Model &model)
    : Page(parent, model)
    , ui(new Ui::RegistersPage) {
  ui->setupUi(this);
  
  QTableWidget* table = this->ui->registers_table;
  table->setRowCount(0);
  
  // // Mantener la conexión para datos en tiempo real si se desea
  // this->connect(&this->model, &Model::sensorDataReceived,
  //               this, [this](const SensorData& data) {
  //                 // Esto es para datos en tiempo real, opcional
  //                 qInfo() << "Realtime sensor received:" << data.distance;
  //                 this->drawSensorsRow(data);
  //               });
}

RegistersPage::~RegistersPage() {
  delete ui;
}

/**
 * @brief Triggered every time the widget is shown.
 *        Reloads all sensor data from the model and redraws the table.
 */
void RegistersPage::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  
  QTableWidget* table = this->ui->registers_table;
  table->setRowCount(0); // Clear previous rows
  
  // Get all stored SensorData from the model
  const auto& sensors = model.getSensorsData();
  qInfo() << "[RegistersPage] Reloading" << sensors.size() << "sensors";
  
  for (const auto& sensor : sensors) {
    drawSensorsRow(sensor);
  }
}

void RegistersPage::drawSensorsRow(const SensorData& sensor) {
  QTableWidget* table = this->ui->registers_table;
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
  
  // Optional: Disable word wrapping for cleaner layout
  table->setWordWrap(false);
}

QVector<QTableWidgetItem*> RegistersPage::sensorDeserialization(
    const SensorData& sensor) {
  
  QVector<QTableWidgetItem*> fields;
  
  auto deserializeToItem = [](float value) {
    auto item = new QTableWidgetItem(QString::number(value));
    item->setTextAlignment(Qt::AlignCenter);
    return item;
  };
  
  qInfo() << "Deserializing sensor" << sensor.distance;
  
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
