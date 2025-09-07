#ifndef PAGE_H
#define PAGE_H

#include "model/model.h"
#include "colors.h"
#include <QSvgRenderer>
#include <QWidget>
#include <QPainter>

class Page : public QWidget {
  Q_OBJECT
protected:
  QSvgRenderer *renderer;
  Model& model;
  
public:
  explicit Page(QWidget *parent, Model& appModel);
signals:
};

#endif // PAGE_H
