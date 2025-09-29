#include "registeruserpage.h"
#include "ui_registeruserpage.h"

RegisterUserPage::RegisterUserPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterUserPage)
{
    ui->setupUi(this);
    // Conectar el botón de registro a la señal
    connect(ui->registerButton, &QPushButton::clicked, this, [this]() {
        QString username = ui->usernameLineEdit->text();
        QString password = ui->passwordLineEdit->text();
        QString rol = ui->rolLineEdit->text();
        emit registerRequested(username, password, rol);
    });
}

RegisterUserPage::~RegisterUserPage()
{
    delete ui;
}
