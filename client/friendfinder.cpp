#include "friendfinder.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QUrl>
#include <QNetworkRequest>
#include <QDebug>

static void showCustomError(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(text);
    msgBox.exec();
}

static void showCustomInfo(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(text);
    msgBox.exec();
}

FriendFinder::FriendFinder(const QString &token, QWidget *parent)
    : QDialog(parent), authToken(token), isFollowing(false)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
}

FriendFinder::~FriendFinder() {}

void FriendFinder::setupUI() {
    setWindowTitle("Find Friends");
    resize(400, 250);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Enter nickname...");
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    resultLabel = new QLabel("", this);
    resultLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(resultLabel);

    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    actionButton = new QPushButton("", this);
    actionButton->setVisible(false);
    layout->addWidget(actionButton);

    connect(searchButton, &QPushButton::clicked, this, &FriendFinder::searchUser);
    connect(actionButton, &QPushButton::clicked, this, &FriendFinder::followUser);
}

void FriendFinder::searchUser() {
    QString login = searchEdit->text().trimmed();
    if (login.isEmpty()) {
        showCustomError(this, "Enter nickname");
        return;
    }
    currentSearchLogin = login;
    QUrl url(QString("http://127.0.0.1:8080/api/users/search?login=%1").arg(login));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, &FriendFinder::onSearchFinished);
}

void FriendFinder::onSearchFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString login = obj["login"].toString();
            QString nickname = obj["nickname"].toString();
            bool followed = obj["followed"].toBool(); // сервер должен возвращать, подписан ли текущий пользователь
            bool mutual = obj["mutual"].toBool();     // взаимная подписка
            resultLabel->setText(QString("User: %1 (%2)").arg(login).arg(nickname));
            statusLabel->setText(mutual ? "You are friends" : (followed ? "You follow" : "Not followed"));
            isFollowing = followed;
            actionButton->setText(followed ? "Unfollow" : "Follow");
            actionButton->setVisible(true);
            // отключаем старое соединение, подключаем нужный слот
            disconnect(actionButton, &QPushButton::clicked, this, nullptr);
            if (followed) {
                connect(actionButton, &QPushButton::clicked, this, &FriendFinder::unfollowUser);
            } else {
                connect(actionButton, &QPushButton::clicked, this, &FriendFinder::followUser);
            }
        } else {
            resultLabel->setText("User not found");
            actionButton->setVisible(false);
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        showCustomError(this, "Search failed: " + errorMsg);
        resultLabel->setText("Error");
        actionButton->setVisible(false);
    }
    reply->deleteLater();
}

void FriendFinder::followUser() {
    QUrl url("http://127.0.0.1:8080/api/users/follow");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &FriendFinder::onFollowFinished);
}

void FriendFinder::onFollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showCustomInfo(this, "You are now following " + currentSearchLogin);
        // Обновляем статус
        isFollowing = true;
        statusLabel->setText("You follow");
        actionButton->setText("Unfollow");
        disconnect(actionButton, &QPushButton::clicked, this, nullptr);
        connect(actionButton, &QPushButton::clicked, this, &FriendFinder::unfollowUser);
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        showCustomError(this, "Follow failed: " + errorMsg);
    }
    reply->deleteLater();
}

void FriendFinder::unfollowUser() {
    QUrl url("http://127.0.0.1:8080/api/users/unfollow");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, &FriendFinder::onUnfollowFinished);
}

void FriendFinder::onUnfollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showCustomInfo(this, "You unfollowed " + currentSearchLogin);
        isFollowing = false;
        statusLabel->setText("Not followed");
        actionButton->setText("Follow");
        disconnect(actionButton, &QPushButton::clicked, this, nullptr);
        connect(actionButton, &QPushButton::clicked, this, &FriendFinder::followUser);
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        showCustomError(this, "Unfollow failed: " + errorMsg);
    }
    reply->deleteLater();
}

void FriendFinder::showError(const QString &message) {
    showCustomError(this, message);
}

void FriendFinder::showInfo(const QString &message) {
    showCustomInfo(this, message);
}