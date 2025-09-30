#include "updateuserpage.h"
#include "ui_updateuserpage.h"

UpdateUserPage::UpdateUserPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UpdateUserPage)
{
    ui->setupUi(this);
}

UpdateUserPage::~UpdateUserPage()
{
    delete ui;
}

void UpdateUserPage::setUser(const User &user)
{
    currentUser = user;
    ui->usernameLineEdit->setText(QString::fromStdString(user.getUsername()));
    ui->nameLineEdit->setText(QString::fromStdString(user.getUsername()));
    ui->roleComboBox->setCurrentText(QString::fromStdString(user.getType()));
}

void UpdateUserPage::on_saveButton_clicked()
{
    currentUser.setUsername(ui->usernameLineEdit->text().toStdString());
    currentUser.setUsername(ui->nameLineEdit->text().toStdString());
    currentUser.setType(ui->roleComboBox->currentText().toStdString());
    emit userUpdated(currentUser);
}

void UpdateUserPage::on_cancelButton_clicked()
{
    emit cancelUpdate();
}
