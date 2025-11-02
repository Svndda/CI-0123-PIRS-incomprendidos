#ifndef SENSORSPAGE_H
#define SENSORSPAGE_H

#include <QTableWidget>
#include "page.h"
#include "sensordata.h"

namespace Ui {
class SensorsPage;
}

class SensorsPage : public Page{
  Q_OBJECT
  
private:
  Ui::SensorsPage *ui;

public:
  explicit SensorsPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~SensorsPage();
  
private:
  void drawArduinesTable();
  void drawSensorsTable();
  QVector<QTableWidgetItem*> sensorDeserialization(const SensorData& sensor);
  QVector<QTableWidgetItem*> arduineDeserialization(const SensorData& sensor);
};

#endif // SENSORSPAGE_H
