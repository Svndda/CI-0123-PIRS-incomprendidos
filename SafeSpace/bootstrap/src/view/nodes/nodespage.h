#ifndef NODESPAGE_H
#define NODESPAGE_H

#include "page.h"
#include <QStandardItemModel>
#include <qtablewidget.h>

struct NodeInfo {
  int id;
  QString name;
  QString ip;
  int port;
  QString status;
};

namespace Ui {
class NodesPage;
}

class NodesPage : public Page {
  Q_OBJECT
  
private:
  Ui::NodesPage *ui;
  QStandardItemModel* logModel;  
  
  QVector<NodeInfo> nodes = {
      {1, "ProxyNode",         "172.17.0.10", 8000, "Apagado"},
      {2, "SafeSpaceServer",   "172.17.0.20", 9000, "Apagado"},
      {3, "ArduinoNode",       "172.17.0.70", 8003, "Apagado"},      
      {4, "VideoStreamNode",   "172.17.0.90", 8554, "Apagado"},
      {5, "MissionPlanner",    "172.17.0.35", 7999, "Apagado"},
      {6, "StorageNode",       "172.17.0.120", 7600, "Apagado"},
      {7, "MapTileServer",     "172.17.0.110", 8080, "Apagado"},
      {8, "EventBusNode",      "172.17.0.140", 8888, "Apagado"}
  };

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
