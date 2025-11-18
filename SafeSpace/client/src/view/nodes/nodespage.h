#ifndef NODESPAGE_H
#define NODESPAGE_H

#include "page.h"

namespace Ui {
class NodesPage;
}

class NodesPage : public Page
{
  Q_OBJECT

public:
  explicit NodesPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~NodesPage();

private:
  Ui::NodesPage *ui;
};

#endif // NODESPAGE_H
