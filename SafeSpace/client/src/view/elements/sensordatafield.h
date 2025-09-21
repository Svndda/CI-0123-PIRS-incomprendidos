#ifndef SENSORDATAFIELD_H
#define SENSORDATAFIELD_H

#include <QWidget>
#include "sensordata.h"

namespace Ui {
class SensorDataField;
}

class SensorDataField : public QWidget {
  Q_OBJECT

public:
  explicit SensorDataField(
      QWidget *parent = nullptr, SensorData data = SensorData()
  );
  ~SensorDataField();

private:
  Ui::SensorDataField *ui;
  const SensorData& sensorData;
};

#endif // SENSORDATAFIELD_H
