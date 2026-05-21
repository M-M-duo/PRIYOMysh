#include "friendfinder.h"
#include "feedwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QScreen>
#include <QUrl>
#include <QVBoxLayout>

const QString API_BASE_URL = "http://127.0.0.1:8080";

static void showMessage(QWidget *parent, const QString &text, QMessageBox::Icon icon) {
    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);

    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    msgBox.move(170, (screenHeight - msgBox.height()) / 2);
    msgBox.exec();
}

static QString extractErrorMessage(QNetworkReply *reply) {
    QByteArray response = reply->readAll();
    QString errorMsg = reply->errorString();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isObject() && doc.object().contains("reason")) {
        errorMsg = doc.object()["reason"].toString();
    }
    return errorMsg;
}

FriendFinder::FriendFinder(const QString &token, QWidget *parent)
    : QDialog(parent), authToken(token), isFollowing(false), isMutual(false) {
    networkManager = new QNetworkAccessManager(this);
    setupUI();
}

FriendFinder::~FriendFinder() {}

void FriendFinder::setupUI() {
    setWindowTitle("PRIYOMYSH");
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
    viewProfileButton = new QPushButton("View profile", this);
    actionButton = new QPushButton("", this);
    resultLayout->addWidget(resultLabel);
    resultLayout->addWidget(viewProfileButton);
    resultLayout->addWidget(actionButton);
    resultWidget->setVisible(false);
    layout->addWidget(resultWidget);

    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    connect(searchButton, &QPushButton::clicked, this, &FriendFinder::searchUser);
    connect(viewProfileButton, &QPushButton::clicked, this, &FriendFinder::onViewProfile);

    connect(actionButton, &QPushButton::clicked, this, [this]() {
        if (isFollowing) {
            unfollowUser();
        } else {
            followUser();
        }
    });
}

void FriendFinder::clearResult() {
    resultWidget->setVisible(false);
    statusLabel->clear();
}

void FriendFinder::searchUser() {
    QString login = searchEdit->text().trimmed();
    if (login.isEmpty()) {
        showMessage(this, "Enter login", QMessageBox::Critical);
        return;
    }
    clearResult();
    currentSearchLogin = login;

    QUrl url(QString("%1/api/friends/search/%2").arg(API_BASE_URL, login));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onSearchFinished(reply); });
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
            statusLabel->setText(mutual ? "You are friends"
                                        : (isFriend ? "Friend request sent" : ""));
            actionButton->setText(isFriend ? "Unfollow" : "Follow");
            resultWidget->setVisible(true);
        } else {
            resultWidget->setVisible(false);
            statusLabel->setText("User not found");
        }
    } else {
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Search failed: " + errorMsg, QMessageBox::Critical);
        clearResult();
    }
    reply->deleteLater();
}

void FriendFinder::followUser() {
    QUrl url(API_BASE_URL + "/api/friends/add");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onFollowFinished(reply); });
}

void FriendFinder::onFollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showMessage(this, "Friend request sent to " + currentSearchLogin, QMessageBox::Information);
        isFollowing = true;
        isMutual = false;
        statusLabel->setText("Friend request sent");
        actionButton->setText("Unfollow");
    } else {
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Add friend failed: " + errorMsg, QMessageBox::Critical);
    }
    reply->deleteLater();
}

void FriendFinder::unfollowUser() {
    QUrl url(API_BASE_URL + "/api/friends/remove");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentSearchLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onUnfollowFinished(reply); });
}

void FriendFinder::onUnfollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showMessage(this, "Friend removed: " + currentSearchLogin, QMessageBox::Information);
        isFollowing = false;
        isMutual = false;
        statusLabel->setText("");
        actionButton->setText("Follow");
    } else {
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Remove friend failed: " + errorMsg, QMessageBox::Critical);
    }
    reply->deleteLater();
}

void FriendFinder::onViewProfile() {
    FeedWindow *userFeed = new FeedWindow(authToken, currentSearchLogin);
    userFeed->show();
    close();
}

void FriendFinder::showError(const QString &message) {
    showMessage(this, message, QMessageBox::Critical);
}