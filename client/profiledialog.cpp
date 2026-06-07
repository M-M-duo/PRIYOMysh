#include "profiledialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>

ProfileDialog::ProfileDialog(const QJsonObject &userData, QWidget *parent)
    : QDialog(parent)
{
    setupUI(userData);
}

void ProfileDialog::setupUI(const QJsonObject &userData) {
    setWindowTitle("Profile");
    setMinimumSize(300, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *form = new QFormLayout();

    form->addRow("Nickname:", new QLabel(userData["nickname"].toString()));
    form->addRow("Login:", new QLabel(userData["login"].toString()));
    form->addRow("Email:", new QLabel(userData["email"].toString()));
    form->addRow("Phone:", new QLabel(userData["phone"].toString()));
    bool isPublic = userData["isPublic"].toBool();
    form->addRow("Public profile:", new QLabel(isPublic ? "Yes" : "No"));

    mainLayout->addLayout(form);
}