#ifndef ADMINISTRATIONPAGE_H
#define ADMINISTRATIONPAGE_H

#include <QWidget>
#include "model/model.h"
#include "page.h"

namespace Ui {
class AdministrationPage;
}

class AdministrationPage : public Page {
  Q_OBJECT

public:
  AdministrationPage(QWidget* parent, Model& model);
  ~AdministrationPage();

private:
  Ui::AdministrationPage *ui;
};

#endif // ADMINISTRATIONPAGE_H
