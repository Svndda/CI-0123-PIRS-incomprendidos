#include "page.h"

Page::Page(QWidget *parent, Model& appModel)
    : QWidget(parent), model(appModel) {
  
  this->renderer = new QSvgRenderer(
      QString(":/images/bg1.svg"), this
  );
}
