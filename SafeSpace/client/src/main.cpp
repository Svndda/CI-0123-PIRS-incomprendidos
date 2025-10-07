// Copyright [2025] Aaron Carmona Sanchez <aaron.carmona@ucr.ac.cr>
#include "controller/appcontroller.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "user.h"
//#include "FileSystem.h"

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
  
  // FileSystem& fs = appController.getFileSystem();
  // UserController userController(&fs);

  
  // // Crear usuarios de la tabla
  // User u1(1, "realAdmin", "Administrador del sistema");
  // u1.setPassword("M2sv8KxpLq");
  // userController.saveUser(u1);

  // User u2(2, "dataAdmin", "Administrador de datos");
  // u2.setPassword("N7vbq2R0");
  // userController.saveUser(u2);

  // User u3(3, "audiTT", "Auditor");
  // u3.setPassword("gH5pxL9pQ");
  // userController.saveUser(u3);

  // User u4(4, "guestAA", "Invitado 1");
  // u4.setPassword("aB7nvZt9Ow1");
  // userController.saveUser(u4);

  // User u5(5, "guestBB", "Invitado 2");
  // u5.setPassword("z9dsRk5Tg");
  // userController.saveUser(u5);

  // // Autenticación de todos los usuarios
  // std::cout << "\n--- Autenticación de usuarios ---\n";
  // std::cout << "realAdmin: ";
  // std::cout << (userController.authenticate("realAdmin", "M2sv8KxpLq") ? "OK" : "Fallo") << std::endl;
  // std::cout << "dataAdmin: ";
  // std::cout << (userController.authenticate("dataAdmin", "N7vbq2R0") ? "OK" : "Fallo") << std::endl;
  // std::cout << "audiTT: ";
  // std::cout << (userController.authenticate("audiTT", "gH5pxL9pQ") ? "OK" : "Fallo") << std::endl;
  // std::cout << "guestAA: ";
  // std::cout << (userController.authenticate("guestAA", "aB7nvZt9Ow1") ? "OK" : "Fallo") << std::endl;
  // std::cout << "guestBB: ";
  // std::cout << (userController.authenticate("guestBB", "z9dsRk5Tg") ? "OK" : "Fallo") << std::endl;

  // /* // Actualizar usuario 'guestAA'
  // std::cout << "\n--- Actualizando usuario guestAA ---\n";
  // bool updated = userController.updateUser("dataAdmin", u2);
  // std::cout << "Actualización: " << (updated ? "Exitosa" : "Fallida") << std::endl;

  // userController.listUsers();*/

  // // Listar usuarios antes de eliminar
  // std::cout << "\n--- Usuarios antes de eliminar ---\n";
  // userController.listUsers();

  // // Eliminar todos los usuarios
  // userController.deleteUser("realAdmin");
  // userController.deleteUser("dataAdmin");
  // userController.deleteUser("audiTT");
  // userController.deleteUser("guestAA");
  // userController.deleteUser("guestBB");

  // // Listar usuarios después de eliminar
  // std::cout << "\n--- Usuarios después de eliminar ---\n";
  // userController.loadUsers();
  // userController.listUsers();
  // bool deleted = userController.deleteUser("testUser");
  // std::cout << "Eliminación: " << (deleted ? "Exitosa" : "Fallida") << std::endl;

  // // 4. Listar usuarios restantes
  
  // userController.listUsers();

  // fs.openFile("UserList");
  // fs.readFile("UserList");
  // fs.closeFile("UserList");
  // fs.printBitMap();
  return a.exec();
}
