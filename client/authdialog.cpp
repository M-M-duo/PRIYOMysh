#include "authdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

AuthDialog::AuthDialog(const QString &mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(mode == "register" ? "Registration" : "Login");

    QVBoxLayout *layout = new QVBoxLayout(this);

    if (mode == "register") {
        layout->addWidget(new QLabel("Nickname:"));
        nicknameEdit = new QLineEdit(this);
        layout->addWidget(nicknameEdit);
    }

    layout->addWidget(new QLabel("Login:"));
    loginEdit = new QLineEdit(this);
    layout->addWidget(loginEdit);

    layout->addWidget(new QLabel("Password:"));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);
}

QString AuthDialog::getNickname() const {
    return nicknameEdit ? nicknameEdit->text() : QString();
}

QString AuthDialog::getLogin() const {
    return loginEdit->text();
}

QString AuthDialog::getPassword() const {
    return passwordEdit->text();
}