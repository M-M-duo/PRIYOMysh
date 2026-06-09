#include "friendfinder.h"
#include "feedwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QPainter>
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

    qDebug() << "Target URL:" << url.toString();
    qDebug() << "Authorization Token:"
             << (authToken.isEmpty()
                     ? "EMPTY"
                     : "Provided (length: " + QString::number(authToken.length()) + ")");
    qDebug() << "Searching for login:" << login;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onSearchFinished(reply); });
}

// void FriendFinder::onSearchFinished(QNetworkReply *reply) {
//     if (reply->error() == QNetworkReply::NoError) {
//         QByteArray response = reply->readAll();
//         QJsonDocument doc = QJsonDocument::fromJson(response);

//         if (doc.isObject()) {
//             QJsonObject obj = doc.object();
//             QString login = obj["login"].toString();
//             bool isMe = obj["isMe"].toBool();

//             if (isMe) {
//                 currentSearchId = "me";
//             } else {
//                 currentSearchId = obj["id"].isString() ? obj["id"].toString()
//                                                        : QString::number(obj["id"].toInt());
//             }

//             bool isFollowed = obj["isFollowed"].toBool();
//             isFollowing = isFollowed;
                
//             resultLabel->setText(login);
//             if (isMe) {
//                 statusLabel->setText("it is you");
//                 actionButton->setVisible(false);
//             } else {
//                 statusLabel->setText(isFollowed ? "You follow" : "Not followed");
//                 actionButton->setText(isFollowed ? "Unfollow" : "Follow");
//                 actionButton->setVisible(true);
//             }
//             resultWidget->setVisible(true);
//         } else {
//             resultWidget->setVisible(false);
//             statusLabel->setText("User not found");
//         }
//     } else {
//         QString errorMsg = extractErrorMessage(reply);
//         showMessage(this, "Search failed: " + errorMsg, QMessageBox::Critical);
//         clearResult();
//     }
//     reply->deleteLater();
// }

void FriendFinder::onSearchFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString login = obj["login"].toString();
            QString avatarBase64 = obj.contains("avatar") ? obj["avatar"].toString() : "";
            bool isMe = obj.contains("isMe") ? obj["isMe"].toBool() : false;
            bool isFollowed = obj.contains("isFollowed") ? obj["isFollowed"].toBool() : false;

            QString userId;
            if (obj["id"].isString()) {
                userId = obj["id"].toString();
            } else if (obj["id"].isDouble()) {
                userId = QString::number(static_cast<long long>(obj["id"].toDouble()));
            }

            qDeleteAll(resultWidget->findChildren<QWidget *>());
            
            QHBoxLayout *layout = new QHBoxLayout(resultWidget);
            layout->setContentsMargins(10, 8, 10, 8);
            layout->setSpacing(15);

            QLabel *avatarLabel = new QLabel();
            avatarLabel->setFixedSize(48, 48);
            
            QPixmap pixmap;
            if (!avatarBase64.isEmpty())
                pixmap.loadFromData(QByteArray::fromBase64(avatarBase64.toLatin1()));
            if (pixmap.isNull())
                pixmap.load(":/sources/default_ava.png");
            if (!pixmap.isNull()) {
                QPixmap scaled =
                    pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPixmap rounded(48, 48);
                rounded.fill(Qt::transparent);
                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setBrush(QBrush(scaled));
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(0, 0, 48, 48, 24, 24);
                avatarLabel->setPixmap(rounded);
            } else {
                avatarLabel->setText("🖼️");
                avatarLabel->setStyleSheet(
                    "background-color: #e0e0e0; border-radius: 24px; color: black;");
                avatarLabel->setAlignment(Qt::AlignCenter);
            }

            QLabel *nameLabel = new QLabel(login);
            nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #c5c5c5;");

            layout->addWidget(avatarLabel);
            layout->addWidget(nameLabel);
            layout->addStretch();

            if (isMe) {
                QLabel *youLabel = new QLabel("It's you ✨");
                youLabel->setStyleSheet("color: #c5c5c5; font-size: 13px; font-style: italic;");
                layout->addWidget(youLabel);
            } else {
                QPushButton *actionBtn = new QPushButton(isFollowed ? "Unfollow" : "Follow");
                actionBtn->setFixedSize(90, 32);
                actionBtn->setStyleSheet(
                    "QPushButton {"
                    "   background-color: rgba(255, 255, 255, 0.07);"
                    "   border-radius: 8px;"
                    "   color: #c5c5c5;"
                    "   font-size: 13px;"
                    "}"
                    "QPushButton:hover { background-color: rgba(255, 255, 255, 0.15); }");
                layout->addWidget(actionBtn);

                connect(actionBtn, &QPushButton::clicked, this, [this, userId, isFollowed]() {});
            }

            resultWidget->setVisible(true);
        } else {
            resultWidget->setVisible(false);
        }
    } else {
        // Обработка ошибок
        resultWidget->setVisible(false);
        QString errorMsg = extractErrorMessage(reply);
        showMessage(this, "Search failed: " + errorMsg, QMessageBox::Critical);
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