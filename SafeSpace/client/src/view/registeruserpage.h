#ifndef REGISTERUSERPAGE_H
#define REGISTERUSERPAGE_H

#include <QString>
#include <QPushButton>
#include "user.h"
#include "page.h"

namespace Ui {
class RegisterUserPage;
}

class RegisterUserPage : public Page {
    Q_OBJECT
public:
  explicit RegisterUserPage(
    QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~RegisterUserPage();

private:
    Ui::RegisterUserPage *ui;
};

#endif // REGISTERUSERPAGE_H
