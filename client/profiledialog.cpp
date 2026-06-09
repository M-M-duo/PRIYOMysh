#include "profiledialog.h"
#include <QFormLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

static QPixmap getRoundedAvatar(const QString &base64, int size = 64) {
    QPixmap pixmap;
    if (base64.isEmpty()) {
        pixmap.load(":/sources/default_ava.png");
    } else {
        pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
    }

    if (pixmap.isNull()) {
        QPixmap empty(size, size);
        empty.fill(Qt::lightGray);
        return empty;
    }

    QPixmap scaled = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(scaled));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, size, size, size / 2, size / 2);

    return rounded;
}

ProfileDialog::ProfileDialog(const QJsonObject &userData, QWidget *parent) : QDialog(parent) {
    setupUI(userData);
}

void ProfileDialog::setupUI(const QJsonObject &userData) {
    setWindowTitle("Profile");
    setMinimumSize(300, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *avatarLabel = new QLabel(this);
    avatarLabel->setAlignment(Qt::AlignCenter);
    QString base64 =
        userData.contains("image") ? userData["image"].toString() : userData["avatar"].toString();
    QPixmap avatar = getRoundedAvatar(base64, 128);
    avatarLabel->setPixmap(avatar);
    mainLayout->addWidget(avatarLabel);

    QFormLayout *form = new QFormLayout();

    form->addRow("Nickname:", new QLabel(userData["nickname"].toString()));
    form->addRow("Login:", new QLabel(userData["login"].toString()));
    form->addRow("Email:", new QLabel(userData["email"].toString()));
    form->addRow("Phone:", new QLabel(userData["phone"].toString()));
    bool isPublic = userData["isPublic"].toBool();
    form->addRow("Public profile:", new QLabel(isPublic ? "Yes" : "No"));

    mainLayout->addLayout(form);
}