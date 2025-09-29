#include "userlistpage.h"
#include "ui_userlistpage.h"

UserListPage::UserListPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserListPage)
{
    ui->setupUi(this);
    // Aquí puedes conectar señales para eliminar/actualizar usuarios
}

UserListPage::~UserListPage()
{
    delete ui;
}
