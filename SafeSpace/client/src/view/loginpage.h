#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include "model/model.h"
#include "page.h"

namespace Ui {
class LoginPage;
}

class LoginPage : public Page {
  Q_OBJECT

public:
  explicit LoginPage(QWidget *parent,
      Model& model);
  ~LoginPage();

private:
  Ui::LoginPage* ui = nullptr;
  
signals:
  void sendCredentials(const User user);
private slots:
  void on_sendCredentials_button_clicked();
};

#endif // LOGIN_H
