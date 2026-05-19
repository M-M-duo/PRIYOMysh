#include "friendfinder.h"
#include "feedwindow.h"
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
    : QDialog(parent), authToken(token), isFollowing(false), isMutual(false)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
}

FriendFinder::~FriendFinder() {}

void FriendFinder::setupUI() {
    setWindowTitle("Find Friends");
    resize(400, 200);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Enter login...");
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    resultWidget = new QWidget(this);
    QHBoxLayout *resultLayout = new QHBoxLayout(resultWidget);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLabel = new QLabel("", this);
    viewPostsButton = new QPushButton("View posts", this);
    actionButton = new QPushButton("", this);
    resultLayout->addWidget(resultLabel);
    resultLayout->addWidget(viewPostsButton);
    resultLayout->addWidget(actionButton);
    resultWidget->setVisible(false);
    layout->addWidget(resultWidget);

    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    connect(searchButton, &QPushButton::clicked, this, &FriendFinder::searchUser);
    connect(viewPostsButton, &QPushButton::clicked, this, &FriendFinder::onViewPosts);
    connect(actionButton, &QPushButton::clicked, this, &FriendFinder::followUser);
}

void FriendFinder::clearResult() {
    resultWidget->setVisible(false);
    statusLabel->clear();
}

void FriendFinder::searchUser() {
    QString login = searchEdit->text().trimmed();
    if (login.isEmpty()) {
        showCustomError(this, "Enter login");
        return;
    }
    clearResult();
    currentSearchLogin = login;
    QUrl url(QString("http://127.0.0.1:8080/api/friends/search?login=%1").arg(login));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onSearchFinished(reply);
    });
}

void FriendFinder::onSearchFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString login = obj["login"].toString();
            bool isFriend = obj["isFriend"].toBool();
            bool mutual = obj["mutual"].toBool();
            currentSearchLogin = login;
            isFollowing = isFriend;
            isMutual = mutual;
            resultLabel->setText(login);
            statusLabel->setText(mutual ? "You are friends" : (isFriend ? "Friend request sent" : ""));
            actionButton->setText(isFriend ? "Unfollow" : "Follow");
            resultWidget->setVisible(true);
            disconnect(actionButton, &QPushButton::clicked, this, nullptr);
            if (isFriend) {
                connect(actionButton, &QPushButton::clicked, this, &FriendFinder::unfollowUser);
            } else {
                connect(actionButton, &QPushButton::clicked, this, &FriendFinder::followUser);
            }
        } else {
            resultWidget->setVisible(false);
            statusLabel->setText("User not found");
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        showCustomError(this, "Search failed: " + errorMsg);
        clearResult();
    }
    reply->deleteLater();
}

void FriendFinder::followUser() {
    QUrl url("http://127.0.0.1:8080/api/friends/add");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onFollowFinished(reply);
    });
}

void FriendFinder::onFollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showCustomInfo(this, "Friend request sent to " + currentSearchLogin);
        isFollowing = true;
        isMutual = false;
        statusLabel->setText("Friend request sent");
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
        showCustomError(this, "Add friend failed: " + errorMsg);
    }
    reply->deleteLater();
}

void FriendFinder::unfollowUser() {
    QUrl url("http://127.0.0.1:8080/api/friends/remove");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onUnfollowFinished(reply);
    });
}

void FriendFinder::onUnfollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showCustomInfo(this, "Friend removed: " + currentSearchLogin);
        isFollowing = false;
        isMutual = false;
        statusLabel->setText("");
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
        showCustomError(this, "Remove friend failed: " + errorMsg);
    }
    reply->deleteLater();
}

void FriendFinder::onViewPosts() {
    FeedWindow *userFeed = new FeedWindow(authToken, currentSearchLogin, this);
    userFeed->show();
}

void FriendFinder::showError(const QString &message) {
    showCustomError(this, message);
}