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
  
  
private:
  Ui::AdministrationPage *ui;

public:
  AdministrationPage(QWidget* parent, Model& model);
  ~AdministrationPage();
  
private slots:
  void registerUser(
    const QString &username, const QString &password, const QString &rol
  );
  
signals:
  void saveUser(
    const QString &username, const QString &password, const QString &rol
  );
};

#endif // ADMINISTRATIONPAGE_H
