#ifndef MACHINESPAGE_H
#define MACHINESPAGE_H

#include "page.h"
#include <QWidget>
#include <qtablewidget.h>

namespace Ui {
class MachinesPage;
}

class MachinesPage : public Page {
  Q_OBJECT
  
private:
  Ui::MachinesPage *ui;
  
public:
  explicit MachinesPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~MachinesPage();
  
private:
  void drawArduinesTable();
  QVector<QTableWidgetItem*> arduineDeserialization();
};

#endif // MACHINESPAGE_H
