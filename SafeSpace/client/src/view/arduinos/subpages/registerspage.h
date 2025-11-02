#ifndef REGISTERSPAGE_H
#define REGISTERSPAGE_H

#include "page.h"
#include <QWidget>

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
};

#endif // REGISTERSPAGE_H
