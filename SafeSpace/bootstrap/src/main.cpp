#include "appcontroller.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  AppController w;
  w.show();
  return a.exec();
}
