#include "registeruserpage.h"
#include "ui_registeruserpage.h"

RegisterUserPage::RegisterUserPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterUserPage)
{
    ui->setupUi(this);
    // Conectar señales de los botones aquí
}

RegisterUserPage::~RegisterUserPage()
{
    delete ui;
}
