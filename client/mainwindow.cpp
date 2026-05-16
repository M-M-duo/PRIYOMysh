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
    msgBox.setWindowTitle("Warning");
    msgBox.setText(text);
    msgBox.exec();
}

static void showCustomError(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("Error");
    msgBox.setText(text);
    msgBox.exec();
}

static void showCustomInfo(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warn_happy.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("Info");
    msgBox.setText(text);
    msgBox.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    networkManager = new QNetworkAccessManager(this);
    setWindowTitle("Authentication");
    setFixedSize(400, 700);
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
        QPixmap scaledLogo = logoPixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    loginBtn->setFixedWidth(370);
    registerBtn->setFixedWidth(370);

    bottomLayout->addStretch();
    bottomLayout->addWidget(loginBtn);
    bottomLayout->addWidget(registerBtn);

    mainLayout->addWidget(bottomWidget);
    setCentralWidget(central);

    connect(loginBtn, &QPushButton::clicked, this, &MainWindow::onSignInClicked);
    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

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
}

MainWindow::~MainWindow() {}

void MainWindow::onSignInClicked() {
    showSignInDialog("login");
}

void MainWindow::onRegisterClicked() {
    showAuthDialog("register");
}

void MainWindow::showAuthDialog(const QString &mode) {
    AuthDialog dialog(mode, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();
        QString email = dialog.getEmail();
        QString phone = dialog.getPhone();
        bool isPublic = dialog.isPublic();

        if (login.isEmpty() || password.isEmpty() || email.isEmpty() || phone.isEmpty()) {
            showCustomWarning(this, "All fields required");
            return;
        }
        sendAuthRequest(login, password, email, phone, isPublic, mode);
    }
}

void MainWindow::showSignInDialog(const QString &mode) {
    AuthDialog dialog(mode, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();
        if (login.isEmpty() || password.isEmpty()) {
            showCustomWarning(this, "Login and password required");
            return;
        }
        sendSignInRequest(login, password);
    }
}

void MainWindow::sendAuthRequest(const QString &login, const QString &password,
                                 const QString &email, const QString &phone, bool isPublic, const QString &mode) {
    QUrl url(QString("http://127.0.0.1:8080/api/auth/%1").arg(mode));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;
    json["email"] = email;
    json["phone"] = phone;
    json["isPublic"] = isPublic;
    json["image"] = "";

    QByteArray data = QJsonDocument(json).toJson();

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << QString::fromUtf8(data);
    qDebug().noquote() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    reply->setProperty("auth_mode", mode);
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
    reply->setProperty("auth_mode", "login");
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onSignInReplyFinished(reply);
    });
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
                close();
            } else {
                showCustomInfo(this, "Registration successful, please sign in");
            }
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        qDebug().noquote() << "===server error=> " << errorMsg;
        showCustomError(this, errorMsg);
    }
    reply->deleteLater();
}

void MainWindow::onSignInReplyFinished(QNetworkReply *reply) {
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
                close();
            } else {
                showCustomError(this, "Token not received");
            }
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        qDebug().noquote() << "===server error=> " << errorMsg;
        showCustomError(this, errorMsg);
    }
    reply->deleteLater();
}