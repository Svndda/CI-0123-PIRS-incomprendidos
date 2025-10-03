#ifndef UPDATEUSERPAGE_H
#define UPDATEUSERPAGE_H

#include <QWidget>
#include "model/structures/user.h"

namespace Ui {
class UpdateUserPage;
}

class UpdateUserPage : public QWidget
{
    Q_OBJECT

public:
    explicit UpdateUserPage(QWidget *parent = nullptr);
    ~UpdateUserPage();

    void setUser(const User &user);

signals:
    void userUpdated(const User &user);
    void cancelUpdate();

private slots:
    void on_saveButton_clicked();
    void on_cancelButton_clicked();

private:
    Ui::UpdateUserPage *ui;
    User currentUser;
};

#endif // UPDATEUSERPAGE_H
