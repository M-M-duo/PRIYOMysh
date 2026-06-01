#include "feedwindow.h"
#include "editprofiledialog.h"
#include "friendfinder.h"
#include "mainwindow.h"
#include "postdialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDialog>
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
#include <QListWidget>
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

static QString wrapText(const QString &text, int maxLineLength) {
    QString result;
    QString currentLine;
    QStringList words = text.split(' ');
    for (const QString &word : words) {
        if (word.length() > maxLineLength) {
            if (!currentLine.isEmpty()) {
                result += currentLine + "\n";
                currentLine.clear();
            }
            int pos = 0;
            while (pos < word.length()) {
                QString part = word.mid(pos, maxLineLength);
                result += part + "\n";
                pos += maxLineLength;
            }
        } else {
            if (currentLine.isEmpty()) {
                currentLine = word;
            } else if (currentLine.length() + 1 + word.length() <= maxLineLength) {
                currentLine += " " + word;
            } else {
                result += currentLine + "\n";
                currentLine = word;
            }
        }
    }
    if (!currentLine.isEmpty()) {
        result += currentLine;
    }
    if (result.endsWith("\n")) {
        result.chop(1);
    }
    return result;
}

class PostWidget : public QWidget {
public:
    PostWidget(const QJsonObject &post, FeedWindow *parent = nullptr)
        : QWidget(parent), feedWindow(parent), currentImageIndex(0) {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(5);
        mainLayout->setContentsMargins(0, 5, 0, 5);
        mainLayout->setAlignment(Qt::AlignCenter);

        QHBoxLayout *authorLayout = new QHBoxLayout();
        authorLayout->setContentsMargins(20, 0, 20, 0);
        authorLayout->setSpacing(10);

        QLabel *avatarLabel = new QLabel(this);
        avatarLabel->setFixedSize(24, 24);
        avatarLabel->setStyleSheet("border-radius: 12px; background-color: #cccccc;");
        if (post.contains("authorAvatar") && !post["authorAvatar"].toString().isEmpty()) {
            QString base64 = post["authorAvatar"].toString();
            QPixmap pixmap;
            pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
            if (!pixmap.isNull()) {
                avatarLabel->setPixmap(
                    pixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                avatarLabel->setStyleSheet("border-radius: 12px;");
            }
        }
        authorLayout->addWidget(avatarLabel);

        authorLabel = new QLabel(post["author"].toString(), this);
        authorLabel->setStyleSheet("font-weight: bold; color: white; text-decoration: none;");
        authorLabel->setCursor(Qt::PointingHandCursor);
        authorLabel->installEventFilter(this);
        authorLayout->addWidget(authorLabel);
        authorLayout->addStretch();
        mainLayout->addLayout(authorLayout);

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

        bool hasImages = !cachedImages.isEmpty();

        if (hasImages) {
            imageLabel = new QLabel(this);
            imageLabel->setFixedSize(400, 400);
            imageLabel->setAlignment(Qt::AlignCenter);
            imageLabel->setScaledContents(false);
            imageLabel->setStyleSheet("border: none; background-color: transparent;");
            updateImage();
            mainLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
        }

        if (hasImages) {
            QHBoxLayout *actionRow = new QHBoxLayout();
            actionRow->setContentsMargins(0, 5, 0, 5);
            actionRow->setSpacing(0);

            actionRow->addStretch();

            prevButton = new QPushButton("◀", this);
            prevButton->setFixedSize(40, 40);
            prevButton->setEnabled(cachedImages.size() > 1 && currentImageIndex > 0);
            actionRow->addWidget(prevButton);

            actionRow->addStretch();

            likeButton = new QPushButton(this);
            dislikeButton = new QPushButton(this);
            likeCount = post["likesCount"].toInt();
            dislikeCount = post["dislikesCount"].toInt();
            likeButton->setText(QString("🧀 %1").arg(likeCount));
            dislikeButton->setText(QString("🪤 %1").arg(dislikeCount));
            likeButton->setStyleSheet("QPushButton { background-color: transparent; border: none; "
                                      "font-size: 16px; padding: 4px; }");
            dislikeButton->setStyleSheet("QPushButton { background-color: transparent; border: "
                                         "none; font-size: 16px; padding: 4px; }");

            QHBoxLayout *likesLayout = new QHBoxLayout();
            likesLayout->setSpacing(10);
            likesLayout->addWidget(likeButton);
            likesLayout->addWidget(dislikeButton);
            actionRow->addLayout(likesLayout);

            actionRow->addStretch();

            nextButton = new QPushButton("▶", this);
            nextButton->setFixedSize(40, 40);
            nextButton->setEnabled(cachedImages.size() > 1 &&
                                   currentImageIndex < cachedImages.size() - 1);
            actionRow->addWidget(nextButton);

            actionRow->addStretch();

            mainLayout->addLayout(actionRow);
        }

        QString wrappedContent = wrapText(post["content"].toString(), 58);
        contentLabel = new QLabel(wrappedContent, this);
        contentLabel->setWordWrap(true);
        contentLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(contentLabel);

        if (!hasImages) {
            QHBoxLayout *infoRow = new QHBoxLayout();
            infoRow->setContentsMargins(20, 5, 20, 5);
            infoRow->setSpacing(10);

            likeButton = new QPushButton(this);
            dislikeButton = new QPushButton(this);
            likeCount = post["likesCount"].toInt();
            dislikeCount = post["dislikesCount"].toInt();
            likeButton->setText(QString("🧀 %1").arg(likeCount));
            dislikeButton->setText(QString("🪤 %1").arg(dislikeCount));
            likeButton->setStyleSheet("QPushButton { background-color: transparent; border: none; "
                                      "font-size: 16px; padding: 4px; }");
            dislikeButton->setStyleSheet("QPushButton { background-color: transparent; border: "
                                         "none; font-size: 16px; padding: 4px; }");

            infoRow->addWidget(likeButton);
            infoRow->addWidget(dislikeButton);
            infoRow->addStretch();

            QString createdAt = post["createdAt"].toString();
            QDateTime dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss.zzz");
            if (!dt.isValid())
                dt = QDateTime::fromString(createdAt, Qt::ISODate);
            dateLabel = new QLabel(dt.toString("dd.MM.yyyy HH:mm"), this);
            dateLabel->setStyleSheet("color: #6c757d;");
            infoRow->addWidget(dateLabel);

            mainLayout->addLayout(infoRow);
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
        mainLayout->addWidget(tagsLabel);

        if (hasImages) {
            QString createdAt = post["createdAt"].toString();
            QDateTime dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss.zzz");
            if (!dt.isValid())
                dt = QDateTime::fromString(createdAt, Qt::ISODate);
            dateLabel = new QLabel("Posted: " + dt.toString("dd.MM.yyyy HH:mm"), this);
            dateLabel->setStyleSheet("color: #6c757d;");
            dateLabel->setAlignment(Qt::AlignCenter);
            mainLayout->addWidget(dateLabel);
        }

        QFrame *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setFixedWidth(440);
        mainLayout->addWidget(line, 0, Qt::AlignCenter);

        author = post["author"].toString();

        if (hasImages) {
            connect(prevButton, &QPushButton::clicked, this, &PostWidget::prevImage);
            connect(nextButton, &QPushButton::clicked, this, &PostWidget::nextImage);
        }
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
            if (prevButton)
                prevButton->setEnabled(currentImageIndex > 0);
            if (nextButton)
                nextButton->setEnabled(currentImageIndex < cachedImages.size() - 1);
        }
    }
    void nextImage() {
        if (currentImageIndex < cachedImages.size() - 1) {
            currentImageIndex++;
            updateImage();
            if (prevButton)
                prevButton->setEnabled(currentImageIndex > 0);
            if (nextButton)
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

FeedWindow::FeedWindow(const QString &token, QWidget *parent)
    : QMainWindow(parent), authToken(token), currentOffset(0), limit(10), friendsFeed(false),
      isOwnProfile(false), isProfileMode(false) {
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    fetchMyLogin();
    loadFeed(false);
}

FeedWindow::~FeedWindow() {}

void FeedWindow::setupUI() {
    setWindowTitle("PRIYOMYSH");
    setFixedSize(450, 840);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    profileHeader = new QWidget(this);
    QVBoxLayout *headerMainLayout = new QVBoxLayout(profileHeader);
    headerMainLayout->setSpacing(10);
    headerMainLayout->setContentsMargins(20, 10, 20, 10);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(10);
    avatarLabel = new QLabel(this);
    avatarLabel->setFixedSize(60, 60);
    avatarLabel->setStyleSheet(
        "border: 1px solid #ccc; border-radius: 30px; background-color: #e0e0e0;");
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setText("🖼️");
    topRow->addWidget(avatarLabel);
    profileLoginLabel = new QLabel("", this);
    profileLoginLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    topRow->addWidget(profileLoginLabel);
    topRow->addStretch();
    headerMainLayout->addLayout(topRow);

    QHBoxLayout *statsRow = new QHBoxLayout();
    statsRow->setSpacing(10);
    statsRow->addStretch();

    followersButton = new QPushButton("0\nfollowers", this);
    followersButton->setFixedSize(80, 80);
    followersButton->setStyleSheet(
        "QPushButton { background-color: rgba(0,0,0,0.1); border: none; border-radius: 8px; }"
        "QPushButton:hover { background-color: rgba(0,0,0,0.3); }");
    followersButton->setCursor(Qt::PointingHandCursor);
    connect(followersButton, &QPushButton::clicked, this, &FeedWindow::onFollowersClicked);
    statsRow->addWidget(followersButton);

    followingButton = new QPushButton("0\nfollowing", this);
    followingButton->setFixedSize(80, 80);
    followingButton->setStyleSheet(
        "QPushButton { background-color: rgba(0,0,0,0.1); border: none; border-radius: 8px; }"
        "QPushButton:hover { background-color: rgba(0,0,0,0.3); }");
    followingButton->setCursor(Qt::PointingHandCursor);
    connect(followingButton, &QPushButton::clicked, this, &FeedWindow::onFollowingClicked);
    statsRow->addWidget(followingButton);

    postsButton = new QPushButton("0\nposts", this);
    postsButton->setFixedSize(80, 80);
    postsButton->setStyleSheet("QPushButton { background-color: rgba(0,0,0,0.1); border: none; "
                               "border-radius: 8px; color: #888; }");
    postsButton->setEnabled(false);
    postsButton->setCursor(Qt::ArrowCursor);
    statsRow->addWidget(postsButton);

    statsRow->addStretch();
    headerMainLayout->addLayout(statsRow);

    followProfileButton = new QPushButton("", this);
    followProfileButton->setFixedHeight(44);
    followProfileButton->setMinimumWidth(width() - 80);
    followProfileButton->setStyleSheet("QPushButton { background-color: rgba(0,0,0,0.1); border: "
                                       "none; border-radius: 10px; font-size: 14px; }"
                                       "QPushButton:hover { background-color: rgba(0,0,0,0.3); }");
    followProfileButton->setCursor(Qt::PointingHandCursor);
    followProfileButton->setVisible(false);
    headerMainLayout->addWidget(followProfileButton, 0, Qt::AlignCenter);

    profileHeader->setVisible(false);
    mainLayout->addWidget(profileHeader);

    scrollArea = new QScrollArea(this);
    scrollWidget = new QWidget();
    postsLayout = new QVBoxLayout(scrollWidget);
    postsLayout->setAlignment(Qt::AlignTop);
    postsLayout->setSpacing(0);
    postsLayout->setContentsMargins(0, 0, 0, 0);
    scrollWidget->setLayout(postsLayout);
    scrollWidget->adjustSize();
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

    backButton = new QPushButton("← Back", this);
    backButton->setFixedSize(100, 48);
    backButton->setVisible(false);
    bottomBar->addWidget(backButton);
    connect(backButton, &QPushButton::clicked, this, &FeedWindow::onBackClicked);

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

    bottomBar->addSpacing(10);
    bottomBar->addWidget(findFriendsButton);
    bottomBar->addSpacing(10);
    bottomBar->addWidget(sharedButton);

    bottomBar->addStretch(1);
    bottomBar->addWidget(createPostButton);
    bottomBar->addStretch(1);

    bottomBar->addWidget(followButton);
    bottomBar->addSpacing(10);
    bottomBar->addWidget(profileButton);
    bottomBar->addSpacing(10);

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
        )")
            .arg(fontSize);
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

    findFriendsButton->setStyleSheet(transparentButtonStyle(40));
    createPostButton->setStyleSheet(transparentButtonStyle(30));
    profileButton->setStyleSheet(transparentButtonStyle(25));
    sharedButton->setStyleSheet(solidButtonStyle);
    followButton->setStyleSheet(solidButtonStyle);
    backButton->setStyleSheet(solidButtonStyle);

    sharedButton->setCheckable(true);
    followButton->setCheckable(true);
    sharedButton->setChecked(!friendsFeed);
    followButton->setChecked(friendsFeed);

    connect(createPostButton, &QPushButton::clicked, this, &FeedWindow::onCreatePost);
    connect(findFriendsButton, &QPushButton::clicked, this, &FeedWindow::onFindFriendsClicked);
    connect(profileButton, &QPushButton::clicked, this, &FeedWindow::onProfileClick);
    connect(sharedButton, &QPushButton::clicked, this, &FeedWindow::onToggleFeedShared);
    connect(followButton, &QPushButton::clicked, this, &FeedWindow::onToggleFeedFollow);
    connect(loadMoreButton, &QPushButton::clicked, this, &FeedWindow::loadMore);
}

void FeedWindow::showFeedButtons() {
    findFriendsButton->setVisible(true);
    sharedButton->setVisible(true);
    createPostButton->setVisible(true);
    followButton->setVisible(true);
    profileButton->setVisible(true);
    backButton->setVisible(false);
    if (followProfileButton)
        followProfileButton->setVisible(false);
}

void FeedWindow::showProfileButtons() {
    findFriendsButton->setVisible(false);
    sharedButton->setVisible(false);
    createPostButton->setVisible(false);
    followButton->setVisible(false);
    profileButton->setVisible(false);
    backButton->setVisible(true);
}

void FeedWindow::resetToMainFeed() {
    isProfileMode = false;
    profileHeader->setVisible(false);
    loadFeed(friendsFeed);
}

void FeedWindow::loadFeed(bool friendsOnly) {
    isProfileMode = false;
    profileHeader->setVisible(false);
    friendsFeed = friendsOnly;
    sharedButton->setChecked(!friendsFeed);
    followButton->setChecked(friendsFeed);
    currentOffset = 0;
    clearPosts();
    showFeedButtons();
    loadPosts(false);
}

void FeedWindow::loadProfile(const QString &login) {
    currentProfileLogin = login;
    isProfileMode = true;
    isOwnProfile = (login == myActualLogin);
    profileHeader->setVisible(false);
    backButton->setVisible(true);
    showProfileButtons();

    QString endpoint = QString("%1/api/profiles/%2").arg(API_BASE_URL, login);
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onProfileInfoFinished(reply); });
}

void FeedWindow::loadMyProfile() {
    loadProfile(myActualLogin);
}

void FeedWindow::fetchMyLogin() {
    QUrl url(API_BASE_URL + "/api/profiles/me");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onMyLoginFinished(reply); });
}

void FeedWindow::onMyLoginFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            myActualLogin = obj["login"].toString();
        } else {
            showCustomMessage(this, "Failed to get own login", ":/sources/warning_01.png");
        }
    } else {
        showCustomMessage(this, "Failed to fetch login: " + reply->errorString(),
                          ":/sources/warning_01.png");
    }
    reply->deleteLater();
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
    profileLoginLabel->setText(profile["login"].toString());
    int followers = profile["followersCount"].toInt();
    int following = profile["followingCount"].toInt();
    int posts = profile["postsCount"].toInt();
    followersButton->setText(QString("%1\nfollowers").arg(followers));
    followingButton->setText(QString("%1\nfollowing").arg(following));
    postsButton->setText(QString("%1\nposts").arg(posts));

    if (profile.contains("image") && !profile["image"].toString().isEmpty()) {
        QString base64 = profile["image"].toString();
        QPixmap pixmap;
        pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
        if (!pixmap.isNull()) {
            avatarLabel->setPixmap(
                pixmap.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            avatarLabel->setStyleSheet("border-radius: 30px;");
        }
    }

    bool isMe = profile.contains("isMe") ? profile["isMe"].toBool() : false;
    if (isOwnProfile || isMe) {
        followProfileButton->setVisible(true);
        followProfileButton->setText("Edit profile");
        followProfileButton->setEnabled(true);
        followProfileButton->setStyleSheet(
            "QPushButton { background-color: rgba(0,0,0,0.1); border: none; border-radius: 10px; "
            "font-size: 14px; }"
            "QPushButton:hover { background-color: rgba(0,0,0,0.3); }");
        disconnect(followProfileButton, &QPushButton::clicked, this, nullptr);
        connect(followProfileButton, &QPushButton::clicked, this,
                &FeedWindow::onEditProfileClicked);
    } else {
        followProfileButton->setVisible(true);
        bool isFollowing = profile["isFollowing"].toBool();
        followProfileButton->setText(isFollowing ? "Unfollow" : "Follow");
        followProfileButton->setEnabled(true);
        followProfileButton->setStyleSheet(
            "QPushButton { background-color: rgba(0,0,0,0.1); border: none; border-radius: 10px; "
            "font-size: 14px; }"
            "QPushButton:hover { background-color: rgba(0,0,0,0.3); }");
        disconnect(followProfileButton, &QPushButton::clicked, this, nullptr);
        if (isFollowing) {
            connect(followProfileButton, &QPushButton::clicked, this,
                    &FeedWindow::onUnfollowFromProfile);
        } else {
            connect(followProfileButton, &QPushButton::clicked, this,
                    &FeedWindow::onFollowFromProfile);
        }
    }
}

void FeedWindow::showNoPostsImage() {
    clearPosts();
    QPixmap noPostsPixmap(":/sources/no_posts.png");
    if (!noPostsPixmap.isNull()) {
        QLabel *imageLabel = new QLabel(this);
        QPixmap scaled =
            noPostsPixmap.scaled(360, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(scaled);
        imageLabel->setAlignment(Qt::AlignCenter);
        postsLayout->addWidget(imageLabel);
    } else {
        QLabel *infoLabel = new QLabel("No posts available", this);
        infoLabel->setAlignment(Qt::AlignCenter);
        postsLayout->addWidget(infoLabel);
    }
}

void FeedWindow::clearPosts() {
    QLayoutItem *child;
    while ((child = postsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_postWidgets.clear();
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
    if (isProfileMode) {
        endpoint = QString("%1/api/posts/feed/%2?limit=%3&offset=%4")
                       .arg(API_BASE_URL, currentProfileLogin)
                       .arg(limit)
                       .arg(currentOffset);
    } else {
        if (friendsFeed) {
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
    loadMyProfile();
}

void FeedWindow::onFindFriendsClicked() {
    FriendFinder dialog(authToken, this);
    dialog.exec();
}

void FeedWindow::onBackClicked() {
    resetToMainFeed();
}

void FeedWindow::onAuthorClicked(const QString &author) {
    if (author == myActualLogin) {
        loadMyProfile();
    } else {
        loadProfile(author);
    }
}

void FeedWindow::onPostReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
        showCustomMessage(this, "Post created successfully", ":/sources/warn_happy.png");
        if (isProfileMode) {
            loadProfile(currentProfileLogin);
        } else {
            loadFeed(friendsFeed);
        }
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
    loadFeed(false);
}

void FeedWindow::onToggleFeedFollow() {
    loadFeed(true);
}

void FeedWindow::onFollowFromProfile() {
    QUrl url(API_BASE_URL + "/api/friends/add");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QJsonObject json;
    json["login"] = currentProfileLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Followed " + currentProfileLogin, ":/sources/warn_happy.png");
            loadProfile(currentProfileLogin);
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
    json["login"] = currentProfileLogin;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Unfollowed " + currentProfileLogin,
                              ":/sources/warn_happy.png");
            loadProfile(currentProfileLogin);
        } else {
            showCustomMessage(this, "Failed to unfollow", ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
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

void FeedWindow::showUserList(const QString &title, const QString &endpoint) {
    QUrl url(API_BASE_URL + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, title]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isArray()) {
                QJsonArray users = doc.array();
                QDialog dialog(this);
                dialog.setWindowTitle(title);
                dialog.resize(300, 400);
                QVBoxLayout *layout = new QVBoxLayout(&dialog);
                QListWidget *listWidget = new QListWidget(&dialog);
                if (users.isEmpty()) {
                    listWidget->addItem("No users found");
                } else {
                    for (const auto &user : users) {
                        if (user.isObject()) {
                            QString login = user.toObject()["login"].toString();
                            listWidget->addItem(login);
                        }
                    }
                }
                layout->addWidget(listWidget);
                QPushButton *closeBtn = new QPushButton("Close", &dialog);
                layout->addWidget(closeBtn);
                connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
                connect(listWidget, &QListWidget::itemClicked,
                        [this, &dialog](QListWidgetItem *item) {
                            QString login = item->text();
                            if (login != "No users found") {
                                dialog.accept();
                                loadProfile(login);
                            }
                        });
                dialog.exec();
            } else {
                showCustomMessage(this, "Invalid response format", ":/sources/warning_01.png");
            }
        } else {
            showCustomMessage(this, "Failed to load " + title + ": " + reply->errorString(),
                              ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
}

void FeedWindow::onFollowersClicked() {
    QString login = isProfileMode ? currentProfileLogin : myActualLogin;
    if (login.isEmpty()) {
        showCustomMessage(this, "Cannot determine login", ":/sources/warning_01.png");
        return;
    }
    QString endpoint = QString("/api/friends/%1/followers").arg(login);
    showUserList("Followers", endpoint);
}

void FeedWindow::onFollowingClicked() {
    QString login = isProfileMode ? currentProfileLogin : myActualLogin;
    if (login.isEmpty()) {
        showCustomMessage(this, "Cannot determine login", ":/sources/warning_01.png");
        return;
    }
    QString endpoint = QString("/api/friends/%1/following").arg(login);
    showUserList("Following", endpoint);
}

void FeedWindow::onEditProfileClicked() {
    qDebug().noquote() << "=== Edit Profile: Fetching current profile data ===";
    QUrl url(API_BASE_URL + "/api/profiles/me");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            qDebug().noquote() << "=== Server response (profile data):";
            qDebug().noquote() << QString::fromUtf8(response);

            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString login = obj["login"].toString();
                QString email = obj.contains("email") ? obj["email"].toString() : "";
                QString phone = obj.contains("phone") ? obj["phone"].toString() : "";
                bool isPublic = obj.contains("isPublic") ? obj["isPublic"].toBool() : true;
                bool isPrivate = !isPublic;
                QString avatarBase64 =
                    obj.contains("image") && !obj["image"].isNull() ? obj["image"].toString() : "";

                EditProfileDialog dialog(login, email, phone, isPrivate, avatarBase64, authToken,
                                         this);
                connect(
                    &dialog, &EditProfileDialog::profileUpdated,
                    [this](const QString &login, const QString &email, const QString &phone,
                           bool isPrivate, const QString &avatarBase64) {
                        qDebug().noquote() << "=== Saving profile updates using PATCH request ===";
                        QUrl updateUrl(API_BASE_URL + "/api/me/profile");
                        QNetworkRequest updateRequest(updateUrl);
                        updateRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                                "application/json");
                        updateRequest.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

                        QJsonObject updateJson;
                        updateJson["login"] = login;
                        if (!email.isEmpty())
                            updateJson["email"] = email;
                        if (!phone.isEmpty())
                            updateJson["phone"] = phone;
                        updateJson["isPublic"] = !isPrivate;
                        if (!avatarBase64.isEmpty()) {
                            updateJson["image"] = avatarBase64;
                        }

                        QByteArray updateData = QJsonDocument(updateJson).toJson();
                        qDebug().noquote() << "=== PATCH request body:";
                        qDebug().noquote() << QString::fromUtf8(updateData);

                        QNetworkReply *updateReply =
                            networkManager->sendCustomRequest(updateRequest, "GET", updateData);
                        qDebug().noquote()
                            << "=== Sending PATCH request to" << updateUrl.toString();
                        connect(updateReply, &QNetworkReply::finished, [this, updateReply]() {
                            if (updateReply->error() == QNetworkReply::NoError) {
                                QByteArray response = updateReply->readAll();
                                qDebug().noquote() << "=== Profile update response:";
                                qDebug().noquote() << QString::fromUtf8(response);
                                showCustomMessage(this, "Profile updated",
                                                  ":/sources/warn_happy.png");
                                loadMyProfile();
                            } else {
                                QByteArray response = updateReply->readAll();
                                QString errorMsg = updateReply->errorString();
                                QJsonDocument doc = QJsonDocument::fromJson(response);
                                if (doc.isObject() && doc.object().contains("reason")) {
                                    errorMsg = doc.object()["reason"].toString();
                                }
                                qDebug().noquote() << "=== Profile update ERROR:";
                                qDebug().noquote() << "HTTP method: PATCH";
                                qDebug().noquote() << "HTTP error:" << updateReply->errorString();
                                qDebug().noquote()
                                    << "Response body:" << QString::fromUtf8(response);
                                showCustomMessage(this, "Update failed: " + errorMsg,
                                                  ":/sources/warning_01.png");
                            }
                            updateReply->deleteLater();
                        });
                    });
                dialog.exec();
            } else {
                qDebug().noquote() << "=== Profile data response is not a JSON object";
            }
        } else {
            QByteArray response = reply->readAll();
            QString errorMsg = reply->errorString();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isObject() && doc.object().contains("reason")) {
                errorMsg = doc.object()["reason"].toString();
            }
            qDebug().noquote() << "=== Failed to fetch profile data:";
            qDebug().noquote() << "HTTP error:" << reply->errorString();
            qDebug().noquote() << "Response body:" << QString::fromUtf8(response);
            showCustomMessage(this, "Failed to load profile data: " + errorMsg,
                              ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
}
void FeedWindow::showError(const QString &message) {
    showCustomMessage(this, message, ":/sources/warning_01.png");
}