#include "editprofiledialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

EditProfileDialog::EditProfileDialog(const QString &login, const QString &email, const QString &phone, bool isPrivate, QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loginEdit->setText(login);
    emailEdit->setText(email);
    phoneEdit->setText(phone);
    privateCheckBox->setChecked(isPrivate);
}

void EditProfileDialog::setupUI() {
    setWindowTitle("Edit Profile");
    setFixedSize(250, 300);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Login:"));
    loginEdit = new QLineEdit(this);
    loginEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; border-radius: 10px; padding: 8px;");
    layout->addWidget(loginEdit);

    layout->addWidget(new QLabel("Email:"));
    emailEdit = new QLineEdit(this);
    emailEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; border-radius: 10px; padding: 8px;");
    layout->addWidget(emailEdit);

    layout->addWidget(new QLabel("Phone:"));
    phoneEdit = new QLineEdit(this);
    phoneEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; border-radius: 10px; padding: 8px;");
    layout->addWidget(phoneEdit);

    privateCheckBox = new QCheckBox("Private profile", this);
    layout->addWidget(privateCheckBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 0, Qt::AlignCenter);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditProfileDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void EditProfileDialog::onSaveClicked() {
    emit profileUpdated(loginEdit->text(), emailEdit->text(), phoneEdit->text(), privateCheckBox->isChecked());
    accept();
}

QString EditProfileDialog::getLogin() const { return loginEdit->text(); }
QString EditProfileDialog::getEmail() const { return emailEdit->text(); }
QString EditProfileDialog::getPhone() const { return phoneEdit->text(); }
bool EditProfileDialog::isPrivate() const { return privateCheckBox->isChecked(); }