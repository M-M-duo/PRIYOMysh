#include "feedwindow.h"
#include "postdialog.h"
#include "profiledialog.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollBar>
#include <QDateTime>
#include <QDebug>

class PostWidget : public QFrame {
public:
    PostWidget(const QJsonObject &post, QWidget *parent = nullptr) : QFrame(parent) {
        setFrameShape(QFrame::Box);
        setStyleSheet("QFrame { border: 1px solid #ddd; border-radius: 8px; margin: 5px; padding: 10px; }");
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *authorLabel = new QLabel(post["author"].toString());
        authorLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
        layout->addWidget(authorLabel);

        QLabel *contentLabel = new QLabel(post["content"].toString());
        contentLabel->setWordWrap(true);
        layout->addWidget(contentLabel);

        QString tagsStr;
        QJsonArray tagsArr = post["tags"].toArray();
        for (const auto &tag : tagsArr) {
            if (!tagsStr.isEmpty()) tagsStr += ", ";
            tagsStr += tag.toString();
        }
        QLabel *tagsLabel = new QLabel("Tags: " + tagsStr);
        tagsLabel->setStyleSheet("color: #007bff; font-size: 12px;");
        layout->addWidget(tagsLabel);

        QString createdAt = post["createdAt"].toString();
        QDateTime dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss.zzz");
        if (!dt.isValid()) dt = QDateTime::fromString(createdAt, Qt::ISODate);
        QLabel *dateLabel = new QLabel("Posted: " + dt.toString("dd.MM.yyyy HH:mm"));
        dateLabel->setStyleSheet("color: #6c757d; font-size: 11px;");
        layout->addWidget(dateLabel);

        QHBoxLayout *likesLayout = new QHBoxLayout();
        QLabel *likesLabel = new QLabel("👍 " + QString::number(post["likesCount"].toInt()));
        QLabel *dislikesLabel = new QLabel("👎 " + QString::number(post["dislikesCount"].toInt()));
        likesLayout->addWidget(likesLabel);
        likesLayout->addWidget(dislikesLabel);
        likesLayout->addStretch();
        layout->addLayout(likesLayout);
    }
};

FeedWindow::FeedWindow(const QString &token, const QJsonObject &userInfo, QWidget *parent)
    : QMainWindow(parent), authToken(token), userData(userInfo), currentOffset(0), limit(10)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    loadPosts(false);
}

FeedWindow::~FeedWindow() {}

void FeedWindow::setupUI() {
    setWindowTitle("Feed");
    resize(600, 900);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *topBar = new QHBoxLayout();
    createPostButton = new QPushButton("Create Post", this);
    profileButton = new QPushButton("👤 Profile", this);
    topBar->addWidget(createPostButton);
    topBar->addStretch();
    topBar->addWidget(profileButton);
    mainLayout->addLayout(topBar);

    scrollArea = new QScrollArea(this);
    scrollWidget = new QWidget();
    postsLayout = new QVBoxLayout(scrollWidget);
    postsLayout->setAlignment(Qt::AlignTop);
    scrollWidget->setLayout(postsLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    loadMoreButton = new QPushButton("Load more", this);
    loadMoreButton->setVisible(false);
    mainLayout->addWidget(loadMoreButton);

    loadingLabel = new QLabel("Loading...", this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setVisible(false);
    mainLayout->addWidget(loadingLabel);

    setCentralWidget(central);

    connect(createPostButton, &QPushButton::clicked, this, &FeedWindow::onCreatePost);
    connect(profileButton, &QPushButton::clicked, this, &FeedWindow::onProfileClick);
    connect(loadMoreButton, &QPushButton::clicked, this, &FeedWindow::loadMore);
}

void FeedWindow::clearPosts() {
    QLayoutItem *child;
    while ((child = postsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
}

void FeedWindow::addPost(const QJsonObject &post) {
    PostWidget *widget = new PostWidget(post);
    postsLayout->addWidget(widget);
}

void FeedWindow::loadPosts(bool append) {
    if (!append) {
        currentOffset = 0;
        clearPosts();
    }
    QUrl url(QString("http://127.0.0.1:8080/api/posts/feed?limit=%1&offset=%2").arg(limit).arg(currentOffset));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    loadingLabel->setVisible(true);
    loadMoreButton->setVisible(false);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onLoadPostsFinished(reply);
    });
}

void FeedWindow::loadMore() {
    currentOffset += limit;
    loadPosts(true);
}

void FeedWindow::onCreatePost() {
    PostDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString description = dialog.getDescription();
        if (description.isEmpty()) {
            QMessageBox::warning(this, "Error", "Description cannot be empty");
            return;
        }
        QUrl url("http://127.0.0.1:8080/api/posts");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());
        QJsonObject json;
        json["content"] = description;
        json["picture"] = "";
        QJsonArray tagsArray;
        for (const QString &tag : dialog.getTags()) {
            tagsArray.append(tag);
        }
        json["tags"] = tagsArray;
        QJsonDocument doc(json);
        QByteArray data = doc.toJson();

        QNetworkReply *reply = networkManager->post(request, data);
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            onPostReplyFinished(reply);
        });
    }
}

void FeedWindow::onProfileClick() {
    QUrl url("http://127.0.0.1:8080/api/users/me");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onProfileInfoFinished(reply);
    });
}

void FeedWindow::onPostReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QMessageBox::information(this, "Success", "Post created");
        loadPosts(false);
    } else {
        showError("Failed to create post: " + reply->errorString());
    }
    reply->deleteLater();
}

void FeedWindow::onProfileInfoFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (!doc.isNull() && doc.isObject()) {
            ProfileDialog dialog(doc.object(), this);
            dialog.exec();
        } else {
            showError("Invalid profile data");
        }
    } else {
        showError("Failed to load profile: " + reply->errorString());
    }
    reply->deleteLater();
}

void FeedWindow::onLoadPostsFinished(QNetworkReply *reply) {
    loadingLabel->setVisible(false);
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isArray()) {
            QJsonArray posts = doc.array();
            for (const auto &post : posts) {
                addPost(post.toObject());
            }
            if (posts.size() == limit) {
                loadMoreButton->setVisible(true);
            } else {
                loadMoreButton->setVisible(false);
            }
        } else {
            showError("Unexpected response format");
        }
    } else {
        showError("Failed to load feed: " + reply->errorString());
        loadMoreButton->setVisible(false);
    }
    reply->deleteLater();
}

void FeedWindow::showError(const QString &message) {
    QMessageBox::critical(this, "Error", message);
}