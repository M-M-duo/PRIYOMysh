#include "mainwindow.h"
#include "authdialog.h"
#include "feedwindow.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonParseError>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

static void showCustomWarning(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    msgBox.exec();
}

static void showCustomError(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    msgBox.exec();
}

static void showCustomInfo(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warn_happy.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    msgBox.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentDialog(nullptr)
{
    networkManager = new QNetworkAccessManager(this);
    setWindowTitle("PRIYOMYSH");
    setFixedSize(420, 840);
    move(0, 0);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *topWidget = new QWidget(this);
    topWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->setAlignment(Qt::AlignCenter);
    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/sources/enter_logo.png");
    if (!logoPixmap.isNull()) {
        QPixmap scaledLogo = logoPixmap.scaled(360, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        logoLabel->setPixmap(scaledLogo);
    } else {
        logoLabel->setText("Logo");
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(logoLabel);
    mainLayout->addWidget(topWidget, 1);

    QWidget *bottomWidget = new QWidget(this);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(15, 0, 15, 10);
    bottomLayout->setSpacing(10);

    QPushButton *loginBtn = new QPushButton("Login", this);
    QPushButton *registerBtn = new QPushButton("Register", this);

    loginBtn->setFixedHeight(48);
    registerBtn->setFixedHeight(48);
    loginBtn->setFixedWidth(390);
    registerBtn->setFixedWidth(390);

    bottomLayout->addStretch();
    bottomLayout->addWidget(loginBtn);
    bottomLayout->addWidget(registerBtn);

    mainLayout->addWidget(bottomWidget);
    setCentralWidget(central);

    QString buttonStyle = R"(
        QPushButton {
            background-color: rgba(200, 200, 200, 0.6);
            border: none;
            border-radius: 18px;
            font-size: 16px;
            font-weight: 500;
            color: #333;
        }
        QPushButton:hover {
            background-color: rgba(180, 180, 180, 0.8);
        }
        QPushButton:pressed {
            background-color: rgba(160, 160, 160, 0.9);
        }
    )";
    loginBtn->setStyleSheet(buttonStyle);
    registerBtn->setStyleSheet(buttonStyle);

    connect(loginBtn, &QPushButton::clicked, [this]() {
        AuthDialog *dialog = new AuthDialog("login", this);
        currentDialog = dialog;
        connect(dialog, &AuthDialog::loginClicked, this, &MainWindow::onLoginClicked);
        dialog->show();
    });

    connect(registerBtn, &QPushButton::clicked, [this]() {
        AuthDialog *dialog = new AuthDialog("register", this);
        currentDialog = dialog;
        connect(dialog, &AuthDialog::registerClicked, this, &MainWindow::onRegisterClicked);
        dialog->show();
    });
}

MainWindow::~MainWindow() {}

void MainWindow::onLoginClicked(const QString &login, const QString &password) {
    pendingLogin = login;
    pendingPassword = password;
    pendingMode = "login";
    sendSignInRequest(login, password);
}

void MainWindow::onRegisterClicked(const QString &login, const QString &password,
                                   const QString &email, const QString &phone, bool isPrivate) {
    pendingLogin = login;
    pendingPassword = password;
    pendingEmail = email;
    pendingPhone = phone;
    pendingIsPrivate = isPrivate;
    pendingMode = "register";
    sendAuthRequest(login, password, email, phone, isPrivate, "register");
}

void MainWindow::sendAuthRequest(const QString &login, const QString &password,
                                 const QString &email, const QString &phone, bool isPrivate, const QString &mode) {
    QUrl url(QString("http://127.0.0.1:8080/api/auth/%1").arg(mode));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;
    json["email"] = email;
    json["phone"] = phone;
    json["isPrivate"] = isPrivate;
    json["image"] = "";

    QByteArray data = QJsonDocument(json).toJson();

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << QString::fromUtf8(data);
    qDebug().noquote() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onAuthReplyFinished(reply);
    });
}

void MainWindow::sendSignInRequest(const QString &login, const QString &password) {
    QUrl url("http://127.0.0.1:8080/api/auth/sign-in");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;

    QByteArray data = QJsonDocument(json).toJson();

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << QString::fromUtf8(data);
    qDebug().noquote() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onAuthReplyFinished(reply);
    });
}

void MainWindow::showErrorForField(const QString &field, const QString &message) {
    if (currentDialog) {
        AuthDialog *dialog = qobject_cast<AuthDialog*>(currentDialog);
        if (dialog) {
            dialog->clearField(field);
            showCustomError(dialog, message);
        } else {
            showCustomError(this, message);
        }
    } else {
        showCustomError(this, message);
    }
}

void MainWindow::onAuthReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("token")) {
                QString token = obj["token"].toString();
                FeedWindow *feed = new FeedWindow(token, QString());
                feed->show();
                if (currentDialog) {
                    currentDialog->accept();
                    currentDialog->deleteLater();
                    currentDialog = nullptr;
                }
                close();
            } else {
                showCustomInfo(this, "Registration successful, please sign in");
                if (currentDialog) {
                    currentDialog->reject();
                    currentDialog->deleteLater();
                    currentDialog = nullptr;
                }
            }
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QString fieldToClear;
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("reason")) {
                errorMsg = obj["reason"].toString();
                QString reason = errorMsg.toLower();
                if (reason.contains("login")) fieldToClear = "login";
                else if (reason.contains("password")) fieldToClear = "password";
                else if (reason.contains("email")) fieldToClear = "email";
                else if (reason.contains("phone")) fieldToClear = "phone";
            }
        }
        qDebug().noquote() << "===server error=> " << errorMsg;
        if (currentDialog) {
            AuthDialog *dialog = qobject_cast<AuthDialog*>(currentDialog);
            if (dialog) {
                if (!fieldToClear.isEmpty()) {
                    dialog->clearField(fieldToClear);
                }
                showCustomError(dialog, errorMsg);
            } else {
                showCustomError(this, errorMsg);
            }
        } else {
            showCustomError(this, errorMsg);
        }
    }
    reply->deleteLater();
}