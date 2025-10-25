#ifndef ARDUINOSPAGE_H
#define ARDUINOSPAGE_H

#include "view/page.h"
#include <QWidget>
#include <qstackedwidget.h>

namespace Ui {
class ArduinosPage;
}

class ArduinosPage : public Page {
  Q_OBJECT
  
private:
  Ui::ArduinosPage *ui;
  QStackedWidget* pageStack;

public:
  explicit ArduinosPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~ArduinosPage();

};

#endif // ARDUINOSPAGE_H
