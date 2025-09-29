#ifndef USERLISTPAGE_H
#define USERLISTPAGE_H

#include <QWidget>

namespace Ui {
class UserListPage;
}

class UserListPage : public QWidget {
    Q_OBJECT
public:
    explicit UserListPage(QWidget *parent = nullptr);
    ~UserListPage();

    // Puedes agregar métodos para actualizar la lista de usuarios

signals:
    void deleteUserRequested(const QString &username);
    void updateUserRequested(const QString &username);

private:
    Ui::UserListPage *ui;
};

#endif // USERLISTPAGE_H
