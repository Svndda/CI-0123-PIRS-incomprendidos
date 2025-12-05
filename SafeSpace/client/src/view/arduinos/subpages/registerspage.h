#ifndef REGISTERSPAGE_H
#define REGISTERSPAGE_H

#include "page.h"
#include <QWidget>
#include <QTableWidgetItem>

namespace Ui {
class RegistersPage;
}

class RegistersPage : public Page {
  Q_OBJECT

public:
  explicit RegistersPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~RegistersPage();

private:
  Ui::RegistersPage *ui;

  void drawSensorsRow(const SensorData& sensor);
  QVector<QTableWidgetItem*> sensorDeserialization(const SensorData& sensor);
  void showEvent(QShowEvent* event);
};

#endif // REGISTERSPAGE_H
