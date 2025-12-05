#ifndef ACCOUNTPAGE_H
#define ACCOUNTPAGE_H

#include "page.h"

namespace Ui {
class AccountPage;
}

class AccountPage : public Page {
  Q_OBJECT

public:
  explicit AccountPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~AccountPage();

private:
  Ui::AccountPage *ui;
  
signals:
  void logout_requested();
};

#endif // ACCOUNTPAGE_H
