#ifndef ROLESCOMBOBOX_H
#define ROLESCOMBOBOX_H

#include <QObject>
#include <QComboBox>
#include <QWheelEvent>

class RolesComboBox : public QComboBox {
  Q_OBJECT
public:
  RolesComboBox(QWidget* parent = nullptr);
  
private:
  void wheelEvent(QWheelEvent* e) override {
    // if (this->hasFocus()) {
    //   QComboBox::wheelEvent(e);  // Solo procesa scroll si tiene foco
    // } else {
    // }
    e->ignore();
    
  }
};

#endif // ROLESCOMBOBOX_H
