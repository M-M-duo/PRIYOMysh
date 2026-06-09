#include "editprofiledialog.h"
#include <QBuffer>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

const QString API_BASE_URL = "http://127.0.0.1:8080";

EditProfileDialog::EditProfileDialog(const QString &login, const QString &email,
                                     const QString &phone, bool isPrivate,
                                     const QString &avatarBase64, const QString &authToken,
                                     QWidget *parent)
    : QDialog(parent), avatarBase64(avatarBase64), authToken(authToken) {
    setupUI();
    loginEdit->setText(login);
    emailEdit->setText(email);
    phoneEdit->setText(phone);
    privateCheckBox->setChecked(isPrivate);
    if (!avatarBase64.isEmpty()) {
        QPixmap pixmap;
        pixmap.loadFromData(QByteArray::fromBase64(avatarBase64.toLatin1()));
        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap rounded(128, 128);
            rounded.fill(Qt::transparent);
            QPainter painter(&rounded);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(QBrush(scaled));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(0, 0, 128, 128, 64, 64);
            avatarLabel->setPixmap(rounded);
            avatarLabel->setStyleSheet("border: none;");
        }
    }
}

EditProfileDialog::~EditProfileDialog() {}

void EditProfileDialog::setupUI() {
    setWindowTitle("Edit Profile");
    setFixedSize(360, 560);
    this->move(45, 180);
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
    avatarLabel->setStyleSheet("border: none; background-color: #e0e0e0;");
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setText("🖼️");
    avatarLabel->setScaledContents(true);
    avatarLabel->setCursor(Qt::PointingHandCursor);
    avatarLabel->installEventFilter(this);
    avatarLayout->addWidget(avatarLabel);
    avatarLayout->addStretch();
    layout->addLayout(avatarLayout);

    layout->addWidget(new QLabel("Login:"));
    loginEdit = new QLineEdit(this);
    loginEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                             "border-radius: 10px; padding: 8px;");
    layout->addWidget(loginEdit);

    layout->addWidget(new QLabel("Email:"));
    emailEdit = new QLineEdit(this);
    emailEdit->setPlaceholderText("example@mail.ru");
    emailEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                             "border-radius: 10px; padding: 8px;");
    layout->addWidget(emailEdit);

    layout->addWidget(new QLabel("Phone:"));
    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("+7 910 294 10 01");
    phoneEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                             "border-radius: 10px; padding: 8px;");
    layout->addWidget(phoneEdit);

    privateCheckBox = new QCheckBox("Private profile", this);
    layout->addWidget(privateCheckBox);

    QString btnStyle = "QPushButton { background-color: rgba(200,200,200,0.6); border: none; "
                       "border-radius: 10px; font-size: 16px; }"
                       "QPushButton:hover { background-color: rgba(180,180,180,0.8); }";

    changePasswordButton = new QPushButton("Change password", this);
    changePasswordButton->setFixedHeight(40);
    changePasswordButton->setStyleSheet(btnStyle);
    layout->addWidget(changePasswordButton);
    connect(changePasswordButton, &QPushButton::clicked, this,
            &EditProfileDialog::onChangePasswordClicked);

    passwordWidget = new QWidget(this);
    passwordWidget->setVisible(false);
    QVBoxLayout *pwdLayout = new QVBoxLayout(passwordWidget);

    pwdLayout->addWidget(new QLabel("Current password:"));
    currentPasswordEdit = new QLineEdit(this);
    currentPasswordEdit->setEchoMode(QLineEdit::Password);
    currentPasswordEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                                       "border-radius: 10px; padding: 8px;");
    pwdLayout->addWidget(currentPasswordEdit);

    pwdLayout->addWidget(new QLabel("New password:"));
    newPasswordEdit = new QLineEdit(this);
    newPasswordEdit->setEchoMode(QLineEdit::Password);
    newPasswordEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                                   "border-radius: 10px; padding: 8px;");
    pwdLayout->addWidget(newPasswordEdit);

    QDialogButtonBox *pwdButtonBox = new QDialogButtonBox(this);
    updatePasswordButton = pwdButtonBox->addButton("Update", QDialogButtonBox::AcceptRole);
    cancelPasswordButton = pwdButtonBox->addButton("Cancel", QDialogButtonBox::RejectRole);

    updatePasswordButton->setFixedSize(80, 32);
    updatePasswordButton->setStyleSheet(btnStyle);
    cancelPasswordButton->setFixedSize(80, 32);
    cancelPasswordButton->setStyleSheet(btnStyle);

    pwdLayout->addWidget(pwdButtonBox, 0, Qt::AlignCenter);
    layout->addWidget(passwordWidget);

    connect(updatePasswordButton, &QPushButton::clicked, this,
            &EditProfileDialog::onPasswordUpdated);
    connect(cancelPasswordButton, &QPushButton::clicked, [this]() {
        passwordWidget->setVisible(false);
        changePasswordButton->setVisible(true);
        currentPasswordEdit->clear();
        newPasswordEdit->clear();
    });

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 0, Qt::AlignCenter);

    QPushButton *saveButton = buttonBox->button(QDialogButtonBox::Save);
    QPushButton *mainCancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    if (saveButton) {
        saveButton->setFixedSize(100, 40);
        saveButton->setStyleSheet(btnStyle);
    }
    if (mainCancelButton) {
        mainCancelButton->setFixedSize(100, 40);
        mainCancelButton->setStyleSheet(btnStyle);
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditProfileDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void EditProfileDialog::chooseAvatar() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Avatar", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty())
        return;
    QImage image(filePath);
    if (image.isNull()) {
        showMessage("Failed to load image", ":/sources/warning_01.png");
        return;
    }
    int size = qMin(image.width(), image.height());
    int x = (image.width() - size) / 2;
    int y = (image.height() - size) / 2;
    QImage cropped = image.copy(x, y, size, size);
    QImage scaled = cropped.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap rounded(128, 128);
    rounded.fill(Qt::transparent);
    QPixmap fromImg = QPixmap::fromImage(scaled);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(fromImg));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 128, 128, 64, 64);
    avatarLabel->setPixmap(rounded);
    avatarLabel->setStyleSheet("border: none;");
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    scaled.save(&buffer, "PNG");
    buffer.close();
    avatarBase64 = QString::fromLatin1(byteArray.toBase64());
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
    changePasswordButton->setVisible(false);
    passwordWidget->setVisible(true);
}
void EditProfileDialog::onUpdatePassword() {
    QString current = currentPasswordEdit->text();
    QString newPass = newPasswordEdit->text();
    QString confirm = confirmPasswordEdit->text();

    if (current.isEmpty() || newPass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "Validation", "All password fields are required.");
        return;
    }
    if (newPass != confirm) {
        QMessageBox::warning(this, "Validation", "New passwords do not match.");
        return;
    }

    QUrl url(API_BASE_URL + "/api/me/password");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["currentPassword"] = current;
    json["newPassword"] = newPass;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->put(request, data);
    connect(reply, &QNetworkReply::finished, this, &EditProfileDialog::onPasswordUpdated);
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
    json["oldPassword"] = currentPwd;
    json["newPassword"] = newPwd;
    QByteArray data = QJsonDocument(json).toJson();

    qDebug() << "=== Password Update Request ===";
    qDebug() << "URL:" << url.toString();
    qDebug() << "Payload:" << QString::fromUtf8(data);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, data);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray response = reply->readAll();
    qDebug() << "=== Password Update Response ===";
    qDebug() << "Status Code:"
             << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "Body:" << QString::fromUtf8(response);

    if (reply->error() == QNetworkReply::NoError) {
        showMessage("Password updated successfully", ":/sources/warn_happy.png");

        emit passwordChanged();
        accept();
    } else {
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        showMessage("Password update failed: " + errorMsg, ":/sources/warning_01.png");
    }

    reply->deleteLater();
}