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

FriendFinder::FriendFinder(const QString &token, FeedWindow *parent)
    : QDialog(parent), authToken(token), parentFeedWindow(parent), isFollowing(false),
      isMutual(false) {
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
    resultLabel->setCursor(Qt::PointingHandCursor);
    resultLabel->installEventFilter(this);
    actionButton = new QPushButton("", this);
    resultLayout->addWidget(resultLabel);
    resultLayout->addWidget(actionButton);

    resultWidget->setVisible(false);
    layout->addWidget(resultWidget);

    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(statusLabel);

    connect(searchButton, &QPushButton::clicked, this, &FriendFinder::searchUser);
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
            currentSearchId =
                obj["id"].isString() ? obj["id"].toString() : QString::number(obj["id"].toInt());

            bool isFollowed = obj["isFollowed"].toBool();
            bool isMe = obj["isMe"].toBool();

            isFollowing = isFollowed;

            resultLabel->setText(login);
            if (isMe) {
                statusLabel->setText("it is you");
                actionButton->setVisible(false);
            } else {
                statusLabel->setText(isFollowed ? "You follow" : "Not followed");
                actionButton->setText(isFollowed ? "Unfollow" : "Follow");
                actionButton->setVisible(true);
            }
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
    json["id"] = currentSearchId;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onFollowFinished(reply); });
}

void FriendFinder::onFollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showMessage(this, "Followed successfully", QMessageBox::Information);
        isFollowing = true;
        statusLabel->setText("You follow");
        actionButton->setText("Unfollow");
    } else {
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Follow failed: " + errorMsg, QMessageBox::Critical);
    }
    reply->deleteLater();
}

void FriendFinder::unfollowUser() {
    QUrl url(API_BASE_URL + "/api/friends/remove");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["id"] = currentSearchId;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onUnfollowFinished(reply); });
}

void FriendFinder::onUnfollowFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showMessage(this, "Unfollowed successfully", QMessageBox::Information);
        isFollowing = false;
        statusLabel->setText("Not followed");
        actionButton->setText("Follow");
    } else {
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Unfollow failed: " + errorMsg, QMessageBox::Critical);
    }
    reply->deleteLater();
}

void FriendFinder::onViewProfile() {
    if (!currentSearchId.isEmpty() && parentFeedWindow) {
        parentFeedWindow->loadProfile(currentSearchId);
        close();
    }
}

bool FriendFinder::eventFilter(QObject *obj, QEvent *event) {
    if (obj == resultLabel && event->type() == QEvent::MouseButtonPress) {
        onViewProfile();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}