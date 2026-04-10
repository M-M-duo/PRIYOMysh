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
    QPixmap original(":/sources/warning_01.png");
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
            showCustomWarning(this, "All fields required");
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
            showCustomWarning(this, "Login and password required");
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

    QByteArray data = QJsonDocument(json).toJson();

    qDebug() << "===client=> " << url.toString();
    qDebug() << QString::fromUtf8(data);
    qDebug() << "";

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

    qDebug() << "===client=> " << url.toString();
    qDebug() << QString::fromUtf8(data);
    qDebug() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    reply->setProperty("auth_mode", "login");
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onSignInReplyFinished(reply);
    });
}

void MainWindow::onAuthReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "===server=> ";
        qDebug() << QString::fromUtf8(response);
        qDebug() << "";
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
        qDebug() << "===server error=> " << reply->errorString();
        showCustomError(this, reply->errorString());
    }
    reply->deleteLater();
}

void MainWindow::onSignInReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug() << "===server=> ";
        qDebug() << QString::fromUtf8(response);
        qDebug() << "";
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
        qDebug() << "===server error=> " << reply->errorString();
        showCustomError(this, reply->errorString());
    }
    reply->deleteLater();
}