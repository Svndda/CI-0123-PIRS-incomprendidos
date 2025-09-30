#ifndef REGISTERUSERPAGE_H
#define REGISTERUSERPAGE_H

#include <QWidget>
#include <QString>
#include <QPushButton>
#include "UserController.h"

namespace Ui {
class RegisterUserPage;
}

class RegisterUserPage : public QWidget {
    Q_OBJECT
public:
    explicit RegisterUserPage(QWidget *parent = nullptr);
    ~RegisterUserPage();
    void on_registerButton_clicked();

signals:
    void registerRequested(const QString &username, const QString &password, const QString &rol);

private:
    Ui::RegisterUserPage *ui;
    UserController* userController;
};

#endif // REGISTERUSERPAGE_H
