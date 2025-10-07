#ifndef REGISTERUSERPAGE_H
#define REGISTERUSERPAGE_H

#include <QString>
#include <QPushButton>
#include "user.h"
#include "page.h"

namespace Ui {
class RegistrationPage;
}

class RegistrationPage : public Page {
    Q_OBJECT
public:
  explicit RegistrationPage(
    QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~RegistrationPage();
  
signals:
  void userRegistrationSuccessful();

private:
    Ui::RegistrationPage *ui;
};

#endif // REGISTERUSERPAGE_H
