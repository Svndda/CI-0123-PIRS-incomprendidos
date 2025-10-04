#ifndef VISUALIZERSPAGE_H
#define VISUALIZERSPAGE_H

#include "page.h"

namespace Ui {
class VisualizersPage;
}

class VisualizersPage : public Page
{
  Q_OBJECT

public:
  explicit VisualizersPage(
      QWidget *parent = nullptr, Model& model = Model::getInstance()
  );
  ~VisualizersPage();

private:
  Ui::VisualizersPage *ui;
};

#endif // VISUALIZERSPAGE_H
