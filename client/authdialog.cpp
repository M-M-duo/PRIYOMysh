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
        layout->addWidget(new QLabel("Email:"));
        emailEdit = new QLineEdit(this);
        layout->addWidget(emailEdit);

        layout->addWidget(new QLabel("Phone:"));
        phoneEdit = new QLineEdit(this);
        layout->addWidget(phoneEdit);

        isPublicCheckBox = new QCheckBox("Public profile", this);
        layout->addWidget(isPublicCheckBox);
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

QString AuthDialog::getLogin() const {
    return loginEdit->text();
}

QString AuthDialog::getPassword() const {
    return passwordEdit->text();
}

QString AuthDialog::getEmail() const {
    return emailEdit ? emailEdit->text() : QString();
}

QString AuthDialog::getPhone() const {
    return phoneEdit ? phoneEdit->text() : QString();
}

bool AuthDialog::isPublic() const {
    return isPublicCheckBox ? isPublicCheckBox->isChecked() : false;
}