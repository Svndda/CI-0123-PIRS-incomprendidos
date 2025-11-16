#ifndef NODESPAGE_H
#define NODESPAGE_H

#include "page.h"

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
  
  QVector<NodeInfo> nodes = {
      {1, "ProxyNode",         "172.17.0.10", 8000, "Running"},
      {2, "SafeSpaceServer",   "172.17.0.20", 9000, "Stopped"},
      {3, "ArduinoNode",       "172.17.0.70", 8003, "Running"},      
      {4, "VideoStreamNode",   "172.17.0.90", 8554, "Running"},
      {5, "MissionPlanner",    "172.17.0.35", 7999, "Stopped"},
      {6, "StorageNode",       "172.17.0.120", 7600, "Running"},
      {7, "MapTileServer",     "172.17.0.110", 8080, "Running"},
      {8, "EventBusNode",      "172.17.0.140", 8888, "Stopped"}
  };
  

public:
  explicit NodesPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~NodesPage();
  
signals:
  void logout();
  
private slots:
  void onShutdownNodeClicked(int row);
  void onStartNodeClicked(int row);
  
};

#endif // NODESPAGE_H
