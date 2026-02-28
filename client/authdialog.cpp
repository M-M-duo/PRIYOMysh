#include "authdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>

AuthDialog::AuthDialog(const QString &mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(mode == "login" ? "logn" : "register");
    setModal(true);
    setFixedSize(300, 150);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *formLayout = new QFormLayout();
    loginEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow("login", loginEdit);
    formLayout->addRow("password:", passwordEdit);

    mainLayout->addLayout(formLayout);

    actionButton = new QPushButton(mode == "login" ? "login" : "register", this);
    connect(actionButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(actionButton);
}

QString AuthDialog::getLogin() const {
    return loginEdit->text();
}

QString AuthDialog::getPassword() const {
    return passwordEdit->text();
}