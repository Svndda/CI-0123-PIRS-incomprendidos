#include "userlistpage.h"
#include "ui_userlistpage.h"

UserListPage::UserListPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserListPage)
{
    ui->setupUi(this);

    // Conectar el botón de eliminar usuario
    connect(ui->deleteUserButton, &QPushButton::clicked, this, [this]() {
        auto selected = ui->userTableWidget->currentRow();
        if (selected >= 0) {
            QString username = ui->userTableWidget->item(selected, 1)->text();
            emit deleteUserRequested(username);
        }
    });
    // Conectar el botón de actualizar usuario (solo emite la señal, lógica a implementar)
    connect(ui->updateUserButton, &QPushButton::clicked, this, [this]() {
        auto selected = ui->userTableWidget->currentRow();
        if (selected >= 0) {
            QString username = ui->userTableWidget->item(selected, 1)->text();
            emit updateUserRequested(username);
        }
    });
}

UserListPage::~UserListPage()
{
    delete ui;
}
