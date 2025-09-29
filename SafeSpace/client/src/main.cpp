// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include "controller/appcontroller.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "UserController.h"
#include "user.h"
#include "FileSystem.h"
int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString &locale : uiLanguages) {
    const QString baseName = "SafeSpace_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  AppController appController;
  appController.show();

  FileSystem fs;
  UserController userController(&fs);

  userController.loadUsers();
  userController.listUsers();
  

  
  fs.openFile("UserList");
  fs.readFile("UserList");
  fs.closeFile("UserList");
  fs.printBitMap();
  return a.exec();
}
