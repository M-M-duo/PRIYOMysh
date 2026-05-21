#include "feedwindow.h"
#include "friendfinder.h"
#include "mainwindow.h"
#include "postdialog.h"
#include <QDateTime>
#include <QDebug>
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QList>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QUrl>
#include <QVBoxLayout>

const QString API_BASE_URL = "http://127.0.0.1:8080";

static void showCustomMessage(QWidget *parent, const QString &text, const QString &iconPath) {
    QMessageBox msgBox(parent);
    QPixmap original(iconPath);
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    msgBox.move(170, (screenHeight - msgBox.height()) / 2);
    msgBox.exec();
}

class PostWidget : public QWidget {
public:
    PostWidget(const QJsonObject &post, FeedWindow *parent = nullptr)
        : QWidget(parent), feedWindow(parent), currentImageIndex(0) {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(5);
        mainLayout->setContentsMargins(0, 5, 0, 5);
        mainLayout->setAlignment(Qt::AlignCenter);

        QHBoxLayout *centerLayout = new QHBoxLayout();
        centerLayout->setContentsMargins(0, 0, 0, 0);
        centerLayout->addStretch();

        QWidget *contentWidget = new QWidget(this);
        QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
        contentLayout->setSpacing(5);
        contentLayout->setAlignment(Qt::AlignCenter);
        contentWidget->setFixedWidth(440);

        postId = post["id"].toString();

        authorLabel = new QLabel(post["author"].toString());
        authorLabel->setStyleSheet(
            "font-weight: bold; color: #007bff; text-decoration: underline;");
        authorLabel->setCursor(Qt::PointingHandCursor);
        authorLabel->installEventFilter(this);
        authorLabel->setAlignment(Qt::AlignCenter);
        contentLayout->addWidget(authorLabel);

        if (post.contains("img") && post["img"].isArray()) {
            QJsonArray imagesArray = post["img"].toArray();
            for (const auto &img : imagesArray) {
                QString base64 = img.toString();
                if (!base64.isEmpty()) {
                    QPixmap pixmap;
                    pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
                    if (!pixmap.isNull()) {
                        cachedImages.append(
                            pixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    }
                }
            }
        }

        if (!cachedImages.isEmpty()) {
            imageLabel = new QLabel(this);
            imageLabel->setFixedSize(400, 400);
            imageLabel->setAlignment(Qt::AlignCenter);
            imageLabel->setScaledContents(false);
            imageLabel->setStyleSheet("border: none; background-color: transparent;");
            updateImage();
            contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
        }

        QHBoxLayout *descriptionRow = new QHBoxLayout();
        descriptionRow->setContentsMargins(0, 0, 0, 0);
        descriptionRow->setSpacing(5);

        if (!cachedImages.isEmpty()) {
            prevButton = new QPushButton("◀", this);
            prevButton->setFixedSize(40, 40);
            prevButton->setEnabled(cachedImages.size() > 1 && currentImageIndex > 0);
            descriptionRow->addWidget(prevButton);
        }

        contentLabel = new QLabel(post["content"].toString());
        contentLabel->setWordWrap(true);
        contentLabel->setAlignment(Qt::AlignCenter);
        contentLabel->setMinimumHeight(80);
        contentLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        descriptionRow->addWidget(contentLabel);

        if (!cachedImages.isEmpty()) {
            nextButton = new QPushButton("▶", this);
            nextButton->setFixedSize(40, 40);
            nextButton->setEnabled(cachedImages.size() > 1 &&
                                   currentImageIndex < cachedImages.size() - 1);
            descriptionRow->addWidget(nextButton);
        }

        contentLayout->addLayout(descriptionRow);

        if (!cachedImages.isEmpty()) {
            connect(prevButton, &QPushButton::clicked, this, &PostWidget::prevImage);
            connect(nextButton, &QPushButton::clicked, this, &PostWidget::nextImage);
        }

        QString tagsStr;
        if (post.contains("tags") && post["tags"].isArray()) {
            QJsonArray tagsArr = post["tags"].toArray();
            for (const auto &tag : tagsArr) {
                if (!tagsStr.isEmpty())
                    tagsStr += " ";
                tagsStr += "#" + tag.toString();
            }
        }
        tagsLabel = new QLabel(tagsStr);
        tagsLabel->setStyleSheet("color: #007bff;");
        tagsLabel->setAlignment(Qt::AlignCenter);
        contentLayout->addWidget(tagsLabel);

        QString createdAt = post["createdAt"].toString();
        QDateTime dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss.zzz");
        if (!dt.isValid())
            dt = QDateTime::fromString(createdAt, Qt::ISODate);
        dateLabel = new QLabel("Posted: " + dt.toString("dd.MM.yyyy HH:mm"));
        dateLabel->setStyleSheet("color: #6c757d;");
        dateLabel->setAlignment(Qt::AlignCenter);
        contentLayout->addWidget(dateLabel);

        QHBoxLayout *actionLayout = new QHBoxLayout();
        actionLayout->setAlignment(Qt::AlignCenter);
        likeButton = new QPushButton(this);
        dislikeButton = new QPushButton(this);
        likeCount = post["likesCount"].toInt();
        dislikeCount = post["dislikesCount"].toInt();
        likeButton->setText(QString("🧀 %1").arg(likeCount));
        dislikeButton->setText(QString("🪤 %1").arg(dislikeCount));
        likeButton->setStyleSheet("QPushButton { background-color: transparent; "
                                  "border: none; font-size: 16px; padding: 4px; }");
        dislikeButton->setStyleSheet(
            "QPushButton { background-color: transparent; border: none; font-size: "
            "16px; padding: 4px; }");
        actionLayout->addWidget(likeButton);
        actionLayout->addWidget(dislikeButton);
        contentLayout->addLayout(actionLayout);

        centerLayout->addWidget(contentWidget);
        centerLayout->addStretch();
        mainLayout->addLayout(centerLayout);

        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setFixedWidth(440);
        mainLayout->addWidget(line, 0, Qt::AlignCenter);

        author = post["author"].toString();

        connect(likeButton, &QPushButton::clicked, [this]() {
            if (feedWindow)
                feedWindow->onLikeDislike(postId, true);
        });
        connect(dislikeButton, &QPushButton::clicked, [this]() {
            if (feedWindow)
                feedWindow->onLikeDislike(postId, false);
        });
    }

    void updateReactions(int likes, int dislikes) {
        likeCount = likes;
        dislikeCount = dislikes;
        likeButton->setText(QString("🧀 %1").arg(likeCount));
        dislikeButton->setText(QString("🪤 %1").arg(dislikeCount));
    }

private slots:
    void prevImage() {
        if (currentImageIndex > 0) {
            currentImageIndex--;
            updateImage();
            prevButton->setEnabled(currentImageIndex > 0);
            nextButton->setEnabled(currentImageIndex < cachedImages.size() - 1);
        }
    }
    void nextImage() {
        if (currentImageIndex < cachedImages.size() - 1) {
            currentImageIndex++;
            updateImage();
            prevButton->setEnabled(currentImageIndex > 0);
            nextButton->setEnabled(currentImageIndex < cachedImages.size() - 1);
        }
    }

private:
    void updateImage() {
        if (cachedImages.isEmpty())
            return;
        imageLabel->setPixmap(cachedImages[currentImageIndex]);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonPress && obj == authorLabel) {
            if (feedWindow) {
                qDebug() << "Author clicked:" << author;
                feedWindow->onAuthorClicked(author);
            }
            return true;
        }
        return QWidget::eventFilter(obj, event);
    }

    QString author;
    QString postId;
    FeedWindow *feedWindow;
    QLabel *authorLabel;
    QList<QPixmap> cachedImages;
    int currentImageIndex;
    QLabel *imageLabel;
    QLabel *contentLabel;
    QLabel *tagsLabel;
    QLabel *dateLabel;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *likeButton;
    QPushButton *dislikeButton;
    int likeCount;
    int dislikeCount;
};

FeedWindow::FeedWindow(const QString &token, const QString &username, QWidget *parent)
    : QMainWindow(parent), authToken(token), currentUsername(username), currentOffset(0), limit(10), followFeed(false), isOwnProfile(false)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    if (username == "me") {
        fetchMyLogin();
    } else if (!username.isEmpty() && username != "me") {
        loadProfileInfo();
    } else {
        loadPosts(false);
    }
}

FeedWindow::~FeedWindow() {}

void FeedWindow::setupUI() {
    setWindowTitle("PRIYOMYSH");
    setFixedSize(450, 840);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    isOwnProfile = (currentUsername == "me");

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    profileHeader = new QWidget(this);
    QVBoxLayout *headerLayout = new QVBoxLayout(profileHeader);
    headerLayout->setAlignment(Qt::AlignCenter);
    profileLoginLabel = new QLabel("", this);
    profileFollowersLabel = new QLabel("", this);
    profileFollowingLabel = new QLabel("", this);
    profilePostsLabel = new QLabel("", this);
    followProfileButton = new QPushButton("", this);
    followProfileButton->setFixedSize(100, 36);
    followProfileButton->setVisible(false);
    headerLayout->addWidget(profileLoginLabel);
    headerLayout->addWidget(profileFollowersLabel);
    headerLayout->addWidget(profileFollowingLabel);
    headerLayout->addWidget(profilePostsLabel);
    headerLayout->addWidget(followProfileButton);
    profileHeader->setVisible(false);
    mainLayout->addWidget(profileHeader);

    scrollArea = new QScrollArea(this);
    scrollWidget = new QWidget();
    postsLayout = new QVBoxLayout(scrollWidget);
    postsLayout->setAlignment(Qt::AlignTop);
    postsLayout->setSpacing(0);
    postsLayout->setContentsMargins(0, 0, 0, 0);
    scrollWidget->setLayout(postsLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea);

    loadMoreButton = new QPushButton("Load more", this);
    loadMoreButton->setFixedHeight(48);
    loadMoreButton->setVisible(false);
    mainLayout->addWidget(loadMoreButton);

    loadingLabel = new QLabel("Loading...", this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setVisible(false);
    mainLayout->addWidget(loadingLabel);

    QHBoxLayout *bottomBar = new QHBoxLayout();
    bottomBar->setContentsMargins(10, 5, 10, 5);

    if (currentUsername.isEmpty()) {
        findFriendsButton = new QPushButton("⌕", this);
        createPostButton = new QPushButton("+", this);
        profileButton = new QPushButton("🐭", this);
        sharedButton = new QPushButton("Shared", this);
        followButton = new QPushButton("Follow", this);

        findFriendsButton->setFixedSize(50, 48);
        createPostButton->setFixedSize(50, 48);
        profileButton->setFixedSize(50, 48);
        sharedButton->setFixedSize(100, 44);
        followButton->setFixedSize(100, 44);

        bottomBar->addStretch();
        bottomBar->addWidget(findFriendsButton);
        bottomBar->addSpacing(10);
        bottomBar->addWidget(sharedButton);
        bottomBar->addSpacing(10);
        bottomBar->addWidget(createPostButton);
        bottomBar->addSpacing(10);
        bottomBar->addWidget(followButton);
        bottomBar->addSpacing(10);
        bottomBar->addWidget(profileButton);
        bottomBar->addStretch();

        connect(createPostButton, &QPushButton::clicked, this, &FeedWindow::onCreatePost);
        connect(findFriendsButton, &QPushButton::clicked, this, &FeedWindow::onFindFriendsClicked);
        connect(profileButton, &QPushButton::clicked, this, &FeedWindow::onProfileClick);
        connect(sharedButton, &QPushButton::clicked, this, &FeedWindow::onToggleFeedShared);
        connect(followButton, &QPushButton::clicked, this, &FeedWindow::onToggleFeedFollow);

        sharedButton->setCheckable(true);
        followButton->setCheckable(true);
        sharedButton->setChecked(!followFeed);
        followButton->setChecked(followFeed);
    } else if (currentUsername == "me") {
        QPushButton *backButton = new QPushButton("← Back", this);
        backButton->setFixedSize(100, 48);
        exitProfileButton = new QPushButton("Exit", this);
        exitProfileButton->setFixedSize(100, 48);
        bottomBar->addWidget(backButton);
        bottomBar->addSpacing(10);
        bottomBar->addWidget(exitProfileButton);
        bottomBar->addStretch();
        connect(backButton, &QPushButton::clicked, this, &FeedWindow::close);
        connect(exitProfileButton, &QPushButton::clicked, this, &FeedWindow::onExitProfile);
    } else {
        QPushButton *backButton = new QPushButton("← Back", this);
        backButton->setFixedSize(100, 48);
        bottomBar->addWidget(backButton);
        bottomBar->addStretch();
        connect(backButton, &QPushButton::clicked, this, &FeedWindow::close);
    }

    QWidget *bottomWidget = new QWidget(this);
    bottomWidget->setLayout(bottomBar);
    bottomWidget->setFixedHeight(60);
    mainLayout->addWidget(bottomWidget);

    setCentralWidget(central);

    auto transparentButtonStyle = [](int fontSize) -> QString {
        return QString(R"(
            QPushButton {
                background-color: transparent;
                border: none;
                border-radius: 18px;
                font-weight: 500;
                font-size: %1px;
                color: white;
            }
            QPushButton:hover {
                background-color: rgba(0, 0, 0, 0.05);
            }
            QPushButton:pressed {
                background-color: rgba(0, 0, 0, 0.1);
            }
        )").arg(fontSize);
    };

    QString solidButtonStyle = R"(
        QPushButton {
            background-color: rgba(200, 200, 200, 0.6);
            border: none;
            border-radius: 18px;
            font-size: 14px;
            font-weight: 500;
            color: #333;
        }
        QPushButton:hover {
            background-color: rgba(180, 180, 180, 0.8);
        }
        QPushButton:checked {
            background-color: rgba(0, 123, 255, 0.7);
            color: white;
        }
    )";

    if (currentUsername.isEmpty()) {
        findFriendsButton->setStyleSheet(transparentButtonStyle(40));
        createPostButton->setStyleSheet(transparentButtonStyle(30));
        profileButton->setStyleSheet(transparentButtonStyle(25));
        sharedButton->setStyleSheet(solidButtonStyle);
        followButton->setStyleSheet(solidButtonStyle);
    } else {
        QPushButton *backButton = qobject_cast<QPushButton *>(bottomBar->itemAt(0)->widget());
        if (backButton)
            backButton->setStyleSheet(solidButtonStyle);
        if (currentUsername == "me" && exitProfileButton) {
            exitProfileButton->setStyleSheet(solidButtonStyle);
        }
    }

    connect(loadMoreButton, &QPushButton::clicked, this, &FeedWindow::loadMore);
}

void FeedWindow::fetchMyLogin() {
    QUrl url(API_BASE_URL + "/api/users/me");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onMyLoginFinished(reply); });
}

// TODO: setup endpoint with /me
void FeedWindow::onMyLoginFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            myActualLogin = obj["login"].toString();
            loadProfileInfo();
        } else {
            showCustomMessage(this, "Failed to get own login", ":/sources/warning_01.png");
        }
    } else {
        showCustomMessage(this, "Failed to fetch login: " + reply->errorString(),
                          ":/sources/warning_01.png");
    }
    reply->deleteLater();
}

void FeedWindow::loadProfileInfo() {
    QString loginToUse =
        (currentUsername == "me" && !myActualLogin.isEmpty()) ? "me" : currentUsername;
    QString endpoint = QString("%1/api/profiles/%2").arg(API_BASE_URL, loginToUse);
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onProfileInfoFinished(reply); });
}

void FeedWindow::onProfileInfoFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject profile = doc.object();
            updateProfileHeader(profile);
            profileHeader->setVisible(true);
            if (!isOwnProfile) {
                followProfileButton->setVisible(true);
                bool isFollowing = profile["isFollowing"].toBool();
                followProfileButton->setText(isFollowing ? "Unfollow" : "Follow");
                disconnect(followProfileButton, &QPushButton::clicked, this, nullptr);
                if (isFollowing) {
                    connect(followProfileButton, &QPushButton::clicked, this,
                            &FeedWindow::onUnfollowFromProfile);
                } else {
                    connect(followProfileButton, &QPushButton::clicked, this,
                            &FeedWindow::onFollowFromProfile);
                }
            }
            bool allowedToSee = profile["allowedToSee"].toBool();
            if (!allowedToSee) {
                showNoPostsImage();
                loadMoreButton->setVisible(false);
            } else {
                loadPosts(false);
            }
        }
    } else {
        showCustomMessage(this, "Failed to load profile", ":/sources/warning_01.png");
    }
    reply->deleteLater();
}

void FeedWindow::updateProfileHeader(const QJsonObject &profile) {
    profileLoginLabel->setText(QString("Login: %1").arg(profile["login"].toString()));
    profileFollowersLabel->setText(QString("Followers: %1").arg(profile["followersCount"].toInt()));
    profileFollowingLabel->setText(QString("Following: %1").arg(profile["followingCount"].toInt()));
    profilePostsLabel->setText(QString("Posts: %1").arg(profile["postsCount"].toInt()));
    profileLoginLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    profileFollowersLabel->setStyleSheet("color: #888;");
    profileFollowingLabel->setStyleSheet("color: #888;");
    profilePostsLabel->setStyleSheet("color: #888;");
}

void FeedWindow::showNoPostsImage() {
    clearPosts();
    QPixmap noPostsPixmap(":/sources/no_posts.png");
    if (!noPostsPixmap.isNull()) {
        QLabel *imageLabel = new QLabel(this);
        QPixmap scaled =
            noPostsPixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(scaled);
        imageLabel->setAlignment(Qt::AlignCenter);
        postsLayout->addWidget(imageLabel);
    } else {
        QLabel *infoLabel = new QLabel("No posts available", this);
        infoLabel->setAlignment(Qt::AlignCenter);
        postsLayout->addWidget(infoLabel);
    }
}

void FeedWindow::onFollowFromProfile() {
    QUrl url(API_BASE_URL + "/api/friends/add");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentUsername;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Followed " + currentUsername, ":/sources/warn_happy.png");
            followProfileButton->setText("Unfollow");
            disconnect(followProfileButton, &QPushButton::clicked, this, nullptr);
            connect(followProfileButton, &QPushButton::clicked, this,
                    &FeedWindow::onUnfollowFromProfile);
        } else {
            showCustomMessage(this, "Failed to follow", ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
}

void FeedWindow::onUnfollowFromProfile() {
    QUrl url(API_BASE_URL + "/api/friends/remove");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentUsername;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Unfollowed " + currentUsername, ":/sources/warn_happy.png");
            followProfileButton->setText("Follow");
            disconnect(followProfileButton, &QPushButton::clicked, this, nullptr);
            connect(followProfileButton, &QPushButton::clicked, this,
                    &FeedWindow::onFollowFromProfile);
        } else {
            showCustomMessage(this, "Failed to unfollow", ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
}

void FeedWindow::clearPosts() {
    QLayoutItem *child;
    while ((child = postsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_postWidgets.clear(); // Теперь очистка идет централизованно
}

void FeedWindow::addPost(const QJsonObject &post) {
    PostWidget *widget = new PostWidget(post, this);
    postsLayout->addWidget(widget);
    m_postWidgets[post["id"].toString()] = widget;
}

void FeedWindow::loadPosts(bool append) {
    if (!append) {
        currentOffset = 0;
        clearPosts();
    }
    QString endpoint;
    if (currentUsername == "me") {
        endpoint = QString("%1/api/posts/feed/my?limit=%2&offset=%3")
                       .arg(API_BASE_URL)
                       .arg(limit)
                       .arg(currentOffset);
    } else if (!currentUsername.isEmpty() && currentUsername != "me") {
        endpoint = QString("%1/api/posts/feed/%2?limit=%3&offset=%4")
                       .arg(API_BASE_URL, currentUsername)
                       .arg(limit)
                       .arg(currentOffset);
    } else {
        if (followFeed) {
            endpoint = QString("%1/api/posts/feed/follow?limit=%2&offset=%3")
                           .arg(API_BASE_URL)
                           .arg(limit)
                           .arg(currentOffset);
        } else {
            endpoint = QString("%1/api/posts/feed?limit=%2&offset=%3")
                           .arg(API_BASE_URL)
                           .arg(limit)
                           .arg(currentOffset);
        }
    }
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << "GET request with Authorization header";
    qDebug().noquote() << "";

    loadingLabel->setVisible(true);
    loadMoreButton->setVisible(false);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onLoadPostsFinished(reply); });
}

void FeedWindow::loadMore() {
    currentOffset += limit;
    loadPosts(true);
}

void FeedWindow::onCreatePost() {
    PostDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString description = dialog.getDescription();
        QStringList images = dialog.getImagesBase64();
        if (description.isEmpty() && images.isEmpty()) {
            showCustomMessage(this, "At least description or image is required",
                              ":/sources/warning_01.png");
            return;
        }
        QUrl url(API_BASE_URL + "/api/posts/new");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

        QJsonObject json;
        json["content"] = description;
        QJsonArray imagesArray;
        for (const QString &img : images) {
            imagesArray.append(img);
        }
        json["img"] = imagesArray;
        QJsonArray tagsArray;
        for (const QString &tag : dialog.getTags()) {
            tagsArray.append(tag);
        }
        json["tags"] = tagsArray;

        QJsonDocument doc(json);
        QByteArray data = doc.toJson();

        qDebug().noquote() << "===client=> " << url.toString();
        qDebug().noquote() << QString::fromUtf8(data);
        qDebug().noquote() << "";

        QNetworkReply *reply = networkManager->post(request, data);
        connect(reply, &QNetworkReply::finished, [this, reply]() { onPostReplyFinished(reply); });
    }
}

void FeedWindow::onProfileClick() {
    FeedWindow *myPostsWindow = new FeedWindow(authToken, "me", this);
    myPostsWindow->show();
}

void FeedWindow::onFindFriendsClicked() {
    FriendFinder dialog(authToken, this);
    dialog.exec();
}

void FeedWindow::onExitProfile() {
    MainWindow *mainWin = new MainWindow();
    mainWin->show();
    close();
}

void FeedWindow::onAuthorClicked(const QString &author) {
    qDebug() << "Opening posts of author:" << author;
    FeedWindow *userFeed = new FeedWindow(authToken, author, this);
    userFeed->show();
}

void FeedWindow::onPostReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
        showCustomMessage(this, "Post created successfully", ":/sources/warn_happy.png");
        loadPosts(false);
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        qDebug().noquote() << "===server error=> " << errorMsg;
        showCustomMessage(this, "Failed to create post: " + errorMsg, ":/sources/warning_01.png");
    }
    reply->deleteLater();
}

void FeedWindow::onLoadPostsFinished(QNetworkReply *reply) {
    loadingLabel->setVisible(false);
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
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
            showCustomMessage(this, "Unexpected response format", ":/sources/warning_01.png");
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        qDebug().noquote() << "===server error=> " << errorMsg;
        showCustomMessage(this, "Failed to load feed: " + errorMsg, ":/sources/warning_01.png");
        loadMoreButton->setVisible(false);
    }
    reply->deleteLater();
}

void FeedWindow::onToggleFeedShared() {
    followFeed = false;
    sharedButton->setChecked(true);
    followButton->setChecked(false);
    loadPosts(false);
}

void FeedWindow::onToggleFeedFollow() {
    followFeed = true;
    sharedButton->setChecked(false);
    followButton->setChecked(true);
    loadPosts(false);
}

void FeedWindow::onLikeDislike(const QString &postId, bool isLike) {
    QString endpoint = isLike ? QString("%1/api/posts/%2/like").arg(API_BASE_URL, postId)
                              : QString("%1/api/posts/%2/dislike").arg(API_BASE_URL, postId);
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, [this, reply, postId]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isObject()) {
                int likes = doc.object()["likesCount"].toInt();
                int dislikes = doc.object()["dislikesCount"].toInt();
                updatePostReaction(postId, likes, dislikes);
            }
        } else {
            qDebug() << "Reaction error:" << reply->errorString();
        }
        reply->deleteLater();
    });
}

void FeedWindow::updatePostReaction(const QString &postId, int newLikes, int newDislikes) {
    if (m_postWidgets.contains(postId)) {
        m_postWidgets[postId]->updateReactions(newLikes, newDislikes);
    }
}

void FeedWindow::showError(const QString &message) {
    showCustomMessage(this, message, ":/sources/warning_01.png");
}