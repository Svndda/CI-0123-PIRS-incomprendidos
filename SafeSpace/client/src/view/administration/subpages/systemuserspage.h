#ifndef SYSTEMUSERSPAGE_H
#define SYSTEMUSERSPAGE_H

#include "page.h"
#include "model.h"
#include <QWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QShowEvent>

namespace Ui {
class SystemUsersPage;
}

class SystemUsersPage : public Page {
  Q_OBJECT
  
public:
  explicit SystemUsersPage(QWidget *parent = nullptr, Model& model = Model::getInstance());
  ~SystemUsersPage();
  
protected:
  void showEvent(QShowEvent *event) override;
  void hideEvent(QHideEvent *event) override;
  
private slots:
  void onSystemUsersReceived(const std::vector<User>& systemUsers);
  
private:
  Ui::SystemUsersPage *ui;
  bool needsRefresh = true;  // Bandera para refrescar al mostrar
  
  void requestUsersFromServer();
};

#endif // SYSTEMUSERSPAGE_H
