#ifndef NODESPAGE_H
#define NODESPAGE_H

#include "page.h"
#include <QStandardItemModel>
#include <qtablewidget.h>

namespace Ui {
class NodesPage;
}

class NodesPage : public Page {
  Q_OBJECT
  
private:
  Ui::NodesPage *ui;
  QStandardItemModel* logModel;
  
  int extractNodeIdFromEvent(const NetworkEvent &evt);
  void applyStatusColor(QTableWidgetItem *item, const QString &state);
public:
  explicit NodesPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~NodesPage();
  
  void updateNodeState(int row, QString state);
  int findRowByNodeId(int nodeId);
  
signals:
  void logout();
  
private slots:
  void onShutdownNodeClicked(int row);
  void onStartNodeClicked(int row);
  void onNetworkEventLogged(const NetworkEvent& evt);
  void addLogToListView(const NetworkEvent& evt);
};

#endif // NODESPAGE_H
