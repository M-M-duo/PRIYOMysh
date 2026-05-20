#include "authdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

AuthDialog::AuthDialog(const QString &mode, QWidget *parent)
    : QDialog(parent), mode(mode)
{
    setupUI();
}

void AuthDialog::setupUI() {
    setWindowTitle("PRIYOMYSH");

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Login:"));
    loginEdit = new QLineEdit(this);
    loginEdit->setStyleSheet("background-color: rgba(200,200,200,0.6); border: none; border-radius: 10px; padding: 8px;");
    layout->addWidget(loginEdit);

    layout->addWidget(new QLabel("Password:"));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("background-color: rgba(200,200,200,0.6); border: none; border-radius: 10px; padding: 8px;");
    layout->addWidget(passwordEdit);

    if (mode == "register") {
        layout->addWidget(new QLabel("Email:"));
        emailEdit = new QLineEdit(this);
        emailEdit->setPlaceholderText("example@mail.ru");
        emailEdit->setStyleSheet("background-color: rgba(200,200,200,0.6); border: none; border-radius: 10px; padding: 8px;");
        layout->addWidget(emailEdit);

        layout->addWidget(new QLabel("Phone:"));
        phoneEdit = new QLineEdit(this);
        phoneEdit->setInputMask("+7-900-000-00-00");
        phoneEdit->setStyleSheet("background-color: rgba(200,200,200,0.6); border: none; border-radius: 10px; padding: 8px;");
        layout->addWidget(phoneEdit);

        privateCheckBox = new QCheckBox("Private profile", this);
        privateCheckBox->setChecked(false);
        layout->addWidget(privateCheckBox);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &AuthDialog::onButtonClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AuthDialog::onButtonClicked() {
    if (mode == "login") {
        emit loginClicked(loginEdit->text(), passwordEdit->text());
    } else {
        emit registerClicked(loginEdit->text(), passwordEdit->text(),
                             emailEdit->text(), phoneEdit->text(),
                             privateCheckBox->isChecked());
    }
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

bool AuthDialog::isPrivate() const {
    return privateCheckBox ? privateCheckBox->isChecked() : false;
}

void AuthDialog::clearField(const QString &fieldName) {
    if (fieldName == "login") {
        loginEdit->clear();
    } else if (fieldName == "password") {
        passwordEdit->clear();
    } else if (fieldName == "email") {
        if (emailEdit) emailEdit->clear();
    } else if (fieldName == "phone") {
        if (phoneEdit) phoneEdit->clear();
    }
}