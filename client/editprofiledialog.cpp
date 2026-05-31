#include "editprofiledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QImage>
#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QGuiApplication>
#include <QEvent>
#include <QDebug>

const QString API_BASE_URL = "http://127.0.0.1:8080";

EditProfileDialog::EditProfileDialog(const QString &login, const QString &email, const QString &phone, bool isPrivate, const QString &avatarBase64, const QString &token, QWidget *parent)
    : QDialog(parent), authToken(token), avatarBase64(avatarBase64) {
    setupUI();
    loginEdit->setText(login);
    emailEdit->setText(email);
    phoneEdit->setText(phone);
    privateCheckBox->setChecked(isPrivate);
    if (!avatarBase64.isEmpty()) {
        QPixmap pixmap;
        pixmap.loadFromData(QByteArray::fromBase64(avatarBase64.toLatin1()));
        if (!pixmap.isNull()) {
            avatarLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            avatarLabel->setStyleSheet("border-radius: 32px;");
        }
    }
}

EditProfileDialog::~EditProfileDialog() {}

void EditProfileDialog::setupUI() {
    setWindowTitle("Edit Profile");
    setFixedSize(300, 450);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QHBoxLayout *avatarLayout = new QHBoxLayout();
    avatarLayout->addStretch();
    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(64, 64);
    avatarLabel->setStyleSheet("border: 1px solid #ccc; border-radius: 32px; background-color: #e0e0e0;");
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setText("🖼️");
    avatarLabel->setCursor(Qt::PointingHandCursor);
    avatarLabel->installEventFilter(this);
    avatarLayout->addWidget(avatarLabel);
    avatarLayout->addStretch();
    layout->addLayout(avatarLayout);

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

    changePasswordButton = new QPushButton("Change password", this);
    layout->addWidget(changePasswordButton);
    connect(changePasswordButton, &QPushButton::clicked, this, &EditProfileDialog::onChangePasswordClicked);

    passwordWidget = new QWidget(this);
    passwordWidget->setVisible(false);
    QVBoxLayout *pwdLayout = new QVBoxLayout(passwordWidget);
    pwdLayout->addWidget(new QLabel("Current password:"));
    currentPasswordEdit = new QLineEdit(this);
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    pwdLayout->addWidget(currentPasswordEdit);
    pwdLayout->addWidget(new QLabel("New password:"));
    newPasswordEdit = new QLineEdit(this);
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    pwdLayout->addWidget(newPasswordEdit);
    QHBoxLayout *pwdButtonLayout = new QHBoxLayout();
    updatePasswordButton = new QPushButton("Update", this);
    cancelPasswordButton = new QPushButton("Cancel", this);
    pwdButtonLayout->addWidget(updatePasswordButton);
    pwdButtonLayout->addWidget(cancelPasswordButton);
    pwdLayout->addLayout(pwdButtonLayout);
    layout->addWidget(passwordWidget);
    connect(updatePasswordButton, &QPushButton::clicked, this, &EditProfileDialog::onPasswordUpdated);
    connect(cancelPasswordButton, &QPushButton::clicked, [this]() {
        passwordWidget->setVisible(false);
        currentPasswordEdit->clear();
        newPasswordEdit->clear();
    });

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 0, Qt::AlignCenter);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditProfileDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void EditProfileDialog::chooseAvatar() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Avatar", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;
    QString base64 = cropAndToBase64(filePath);
    if (base64.isEmpty()) {
        showMessage("Failed to load image", ":/sources/warning_01.png");
        return;
    }
    avatarBase64 = base64;
    QPixmap pixmap;
    pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
    if (!pixmap.isNull()) {
        avatarLabel->setPixmap(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        avatarLabel->setStyleSheet("border-radius: 32px;");
    }
}

QString EditProfileDialog::cropAndToBase64(const QString &filePath) {
    QImage image(filePath);
    if (image.isNull()) return QString();
    int size = qMin(image.width(), image.height());
    int x = (image.width() - size) / 2;
    int y = (image.height() - size) / 2;
    QImage cropped = image.copy(x, y, size, size);
    const int maxSize = 64;
    if (cropped.width() > maxSize || cropped.height() > maxSize) {
        cropped = cropped.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    cropped.save(&buffer, "JPEG", 85);
    buffer.close();
    return QString::fromLatin1(byteArray.toBase64());
}

void EditProfileDialog::showMessage(const QString &text, const QString &iconPath) {
    QMessageBox msgBox(this);
    QPixmap original(iconPath);
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    msgBox.move(170, (screenHeight - msgBox.height()) / 2);
    msgBox.exec();
}

bool EditProfileDialog::eventFilter(QObject *obj, QEvent *event) {
    if (obj == avatarLabel && event->type() == QEvent::MouseButtonPress) {
        chooseAvatar();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void EditProfileDialog::onSaveClicked() {
    emit profileUpdated(loginEdit->text(), emailEdit->text(), phoneEdit->text(),
                        privateCheckBox->isChecked(), avatarBase64);
    accept();
}

void EditProfileDialog::onChangePasswordClicked() {
    passwordWidget->setVisible(true);
}

void EditProfileDialog::onPasswordUpdated() {
    QString currentPwd = currentPasswordEdit->text();
    QString newPwd = newPasswordEdit->text();
    if (currentPwd.isEmpty() || newPwd.isEmpty()) {
        showMessage("Please fill both fields", ":/sources/warning_01.png");
        return;
    }
    QUrl url(API_BASE_URL + "/api/me/updatePassword");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + authToken).toUtf8());

    QJsonObject json;
    json["currentPassword"] = currentPwd;
    json["newPassword"] = newPwd;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply, manager]() {
        if (reply->error() == QNetworkReply::NoError) {
            showMessage("Password updated successfully", ":/sources/warn_happy.png");
            passwordWidget->setVisible(false);
            currentPasswordEdit->clear();
            newPasswordEdit->clear();
        } else {
            QByteArray response = reply->readAll();
            QString errorMsg = reply->errorString();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isObject() && doc.object().contains("reason")) {
                errorMsg = doc.object()["reason"].toString();
            }
            showMessage("Password update failed: " + errorMsg, ":/sources/warning_01.png");
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}