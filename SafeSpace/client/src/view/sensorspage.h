#ifndef SENSORSPAGE_H
#define SENSORSPAGE_H

#include "page.h"

namespace Ui {
class SensorsPage;
}

class SensorsPage : public Page{
  Q_OBJECT

public:
  explicit SensorsPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()\
  );
  ~SensorsPage();

private:
  Ui::SensorsPage *ui;
};

#endif // SENSORSPAGE_H
