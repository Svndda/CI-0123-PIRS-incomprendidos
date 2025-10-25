#ifndef SYSTEMUSERSPAGE_H
#define SYSTEMUSERSPAGE_H

#include "page.h"

namespace Ui {
class SystemUsersPage;
}

class SystemUsersPage : public Page {
  Q_OBJECT
  
private:
  Ui::SystemUsersPage *ui;
  
public:
  explicit SystemUsersPage(QWidget *parent = nullptr, Model& model = Model::getInstance());
  ~SystemUsersPage();
    
private:
  void refreshUsersTable();

signals:
  void deleteUserRequested(const QString &username);
  void updateUserRequested(const QString &username);
  
public slots:
  void usersModified();
};

#endif // SYSTEMUSERSPAGE_H
