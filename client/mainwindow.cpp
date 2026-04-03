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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    networkManager = new QNetworkAccessManager(this);
    setWindowTitle("Authentication");
    resize(400, 300);

    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    QPushButton *loginBtn = new QPushButton("Login", this);
    QPushButton *registerBtn = new QPushButton("Register", this);
    layout->addWidget(loginBtn);
    layout->addWidget(registerBtn);
    setCentralWidget(central);

    connect(loginBtn, &QPushButton::clicked, this, &MainWindow::onSignInClicked);
    connect(registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
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
        QString nickname = dialog.getNickname();
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();
        QString email = dialog.getEmail();
        QString phone = dialog.getPhone();
        bool isPublic = dialog.isPublic();

        if (login.isEmpty() || password.isEmpty() || nickname.isEmpty() || email.isEmpty() || phone.isEmpty()) {
            QMessageBox::warning(this, "Error", "All fields required");
            return;
        }
        sendAuthRequest(nickname, login, password, email, phone, isPublic, mode);
    }
}

void MainWindow::showSignInDialog(const QString &mode) {
    AuthDialog dialog(mode, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Error", "Login and password required");
            return;
        }
        sendSignInRequest(login, password);
    }
}

void MainWindow::sendAuthRequest(const QString &nickname, const QString &login, const QString &password,
                                 const QString &email, const QString &phone, bool isPublic, const QString &mode) {
    QUrl url(QString("http://127.0.0.1:8080/api/auth/%1").arg(mode));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;
    json["nickname"] = nickname;
    json["email"] = email;
    json["phone"] = phone;
    json["isPublic"] = isPublic;
    json["image"] = "";

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
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

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    reply->setProperty("auth_mode", "login");
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onSignInReplyFinished(reply);
    });
}

void MainWindow::onAuthReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "Auth response:" << response;
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("token")) {
                QString token = obj["token"].toString();
                FeedWindow *feed = new FeedWindow(token, QJsonObject());
                feed->show();
                close();
            } else {
                QMessageBox::information(this, "Registration", "Success, please sign in");
            }
        }
    } else {
        QMessageBox::critical(this, "Error", reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::onSignInReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "Sign-in response:" << response;
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("token")) {
                QString token = obj["token"].toString();
                FeedWindow *feed = new FeedWindow(token, QJsonObject());
                feed->show();
                close();
            } else {
                QMessageBox::critical(this, "Error", "Token not received");
            }
        }
    } else {
        QMessageBox::critical(this, "Error", reply->errorString());
    }
    reply->deleteLater();
}