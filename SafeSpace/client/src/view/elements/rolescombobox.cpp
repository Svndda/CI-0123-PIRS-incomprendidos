#include "rolescombobox.h"

RolesComboBox::RolesComboBox(QWidget* parent) : QComboBox(parent) {
  this->addItem("Administrador");
  this->addItem("Auditor");
  this->addItem("Soporte");
  this->addItem("Consultor");
}
