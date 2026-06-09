#include "feedwindow.h"
#include "editprofiledialog.h"
#include "friendfinder.h"
#include "mainwindow.h"
#include "postdialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QEvent>
#include <QEventLoop>
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
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QUrl>
#include <QVBoxLayout>

const QString API_BASE_URL = "http://127.0.0.1:8080";

static QPixmap getRoundedAvatar(const QString &base64, int size = 64) {
    QPixmap pixmap;
    if (base64.isEmpty()) {
        pixmap.load(":/sources/default_ava.png");
    } else {
        pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
    }

    if (pixmap.isNull()) {
        QPixmap empty(size, size);
        empty.fill(Qt::lightGray);
        return empty;
    }

    QPixmap scaled = pixmap.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap rounded(size, size);
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(scaled));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, size, size, size / 2, size / 2);

    return rounded;
}

static void showCustomInfo(QWidget *parent, const QString &text) {
    QDialog dialog(parent);
    dialog.setWindowTitle("PRIYOMYSH");
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *iconLabel = new QLabel();
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(190, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconLabel->setPixmap(scaled);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel *textLabel = new QLabel(text);
    textLabel->setStyleSheet("color: white; font-size: 16px;");
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: "
                         "8px; padding: 8px 20px; font-size: 14px; }"
                         "QPushButton:hover { background-color: #0056b3; }");
    layout->addWidget(okBtn, 0, Qt::AlignCenter);

    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

static void showCustomMessage(QWidget *parent, const QString &text, const QString &iconPath) {
    QDialog dialog(parent);
    dialog.setWindowTitle("PRIYOMYSH");
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; }");
    dialog.setFixedSize(360, 400);

    QScreen *screen = QGuiApplication::primaryScreen();
    int screenWidth = screen->availableGeometry().width();
    int screenHeight = screen->availableGeometry().height();
    dialog.move((screenWidth - 360) / 2, (screenHeight - 400) / 2);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *iconLabel = new QLabel();
    QPixmap original(":/sources/warn_happy.png");
    QPixmap scaled = original.scaled(190, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconLabel->setPixmap(scaled);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel *textLabel = new QLabel(text);
    textLabel->setStyleSheet("color: white; font-size: 16px;");
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: "
                         "8px; padding: 8px 20px; font-size: 14px; }"
                         "QPushButton:hover { background-color: #0056b3; }");
    layout->addWidget(okBtn, 0, Qt::AlignCenter);

    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
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

class UserListItem : public QWidget {
public:
    UserListItem(const QString &username, const QString &avatarBase64, QWidget *parent = nullptr)
        : QWidget(parent) {

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(15);

        QLabel *avatarLabel = new QLabel(this);
        avatarLabel->setFixedSize(48, 48);
        avatarLabel->setPixmap(getRoundedAvatar(avatarBase64, 48));
        avatarLabel->setStyleSheet("border: none;");

        QLabel *nameLabel = new QLabel(username, this);
        nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #c5c5c5;");

        layout->addWidget(avatarLabel);
        layout->addWidget(nameLabel);
        layout->addStretch();
    }
};

class PostWidget : public QWidget {
public:
    PostWidget(const QJsonObject &post, FeedWindow *parent = nullptr)
        : QWidget(parent), feedWindow(parent), currentImageIndex(0) {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(5);
        mainLayout->setContentsMargins(0, 5, 0, 5);
        mainLayout->setAlignment(Qt::AlignCenter);

        postId =
            post["id"].isString() ? post["id"].toString() : QString::number(post["id"].toInt());
        authorId = post["author_id"].isString() ? post["author_id"].toString()
                                                : QString::number(post["author_id"].toInt());
        isMePost = post["isMe"].toBool();

        QHBoxLayout *authorLayout = new QHBoxLayout();
        authorLayout->setContentsMargins(20, 0, 20, 0);
        authorLayout->setSpacing(10);

        QLabel *avatarLabel = new QLabel(this);
        avatarLabel->setFixedSize(32, 32);
        avatarLabel->setScaledContents(true);
        QString base64 = post["author_avatar"].toString();
        avatarLabel->setPixmap(getRoundedAvatar(base64, 48));
        avatarLabel->setStyleSheet("border: none;");
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
                QString imgBase64 = img.toString();
                if (!imgBase64.isEmpty()) {
                    QPixmap pixmap;
                    pixmap.loadFromData(QByteArray::fromBase64(imgBase64.toLatin1()));
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
        line->setFixedWidth(420);
        mainLayout->addWidget(line, 0, Qt::AlignCenter);

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
                feedWindow->onAuthorClicked(authorId, isMePost);
            }
            return true;
        }
        return QWidget::eventFilter(obj, event);
    }

    QString authorId;
    QString postId;
    bool isMePost;
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
    : QMainWindow(parent), authToken(token), limit(10), friendsFeed(false), isProfileMode(false) {
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    loadFeed(false);
}

FeedWindow::~FeedWindow() {}

void FeedWindow::setupUI() {
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
    avatarLabel->setStyleSheet("border: none;");
    avatarLabel->setPixmap(getRoundedAvatar("", 120));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setScaledContents(true);
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

    exitButton = new QPushButton("Exit", this);
    exitButton->setFixedSize(100, 48);
    exitButton->setStyleSheet(
        "QPushButton { background-color: #007bff; border: none; border-radius: 18px; font-size: "
        "14px; font-weight: 500; color: white; }"
        "QPushButton:hover { background-color: #0056b3; }"
        "QPushButton:pressed { background-color: #004085; }");
    exitButton->setVisible(false);
    bottomBar->addWidget(exitButton);
    connect(exitButton, &QPushButton::clicked, this, &FeedWindow::logoutRequested);

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
    exitButton->setVisible(false);
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
    exitButton->setVisible(currentProfileId == "me");
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
    lastPostDate.clear();
    lastPostId.clear();
    clearPosts();
    showFeedButtons();
    loadPosts(false);
}

void FeedWindow::loadProfile(const QString &id) {
    currentProfileId = id;
    isProfileMode = true;
    profileHeader->setVisible(false);
    showProfileButtons();

    QString endpoint = QString("%1/api/profiles/%2").arg(API_BASE_URL, id);
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onProfileInfoFinished(reply); });
}

void FeedWindow::loadMyProfile() {
    loadProfile("me");
}

void FeedWindow::onProfileInfoFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject profile = doc.object();
            currentProfileLogin = profile["login"].toString();

            updateProfileHeader(profile);

            if (profile.contains("id")) {
                currentProfileId = profile["id"].isString()
                                       ? profile["id"].toString()
                                       : QString::number(profile["id"].toInt());
            }

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

    QString base64 = profile["image"].toString();
    avatarLabel->setPixmap(getRoundedAvatar(base64, 120));
    avatarLabel->setStyleSheet("border: none;");

    bool isMe = profile.contains("isMe") ? profile["isMe"].toBool() : false;
    if (currentProfileId == "me" || isMe) {
        followProfileButton->setVisible(true);
        followProfileButton->setFixedWidth(390);
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
        followProfileButton->setFixedWidth(420);
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

    QLabel *imageLabel = new QLabel(this);
    QPixmap noPostsPixmap(":/sources/no_posts.png");

    if (!noPostsPixmap.isNull()) {
        QPixmap scaled =
            noPostsPixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imageLabel->setPixmap(scaled);
    } else {
        imageLabel->setText("No posts available");
        imageLabel->setStyleSheet("color: #6c757d; font-size: 16px;");
    }

    imageLabel->setAlignment(Qt::AlignCenter);
    postsLayout->addWidget(imageLabel);
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
    QString pId =
        post["id"].isString() ? post["id"].toString() : QString::number(post["id"].toInt());
    m_postWidgets[pId] = widget;
}

void FeedWindow::loadPosts(bool append) {
    if (!append) {
        lastPostDate.clear();
        lastPostId.clear();
        clearPosts();
    }

    QString cursorParam;
    if (append && !lastPostId.isEmpty() && !lastPostDate.isEmpty()) {
        cursorParam = QString("&cursor=%1:%2").arg(lastPostDate, lastPostId);
    }

    QString endpoint;
    if (isProfileMode) {
        endpoint = QString("%1/api/posts/feed/%2?limit=%3%4")
                       .arg(API_BASE_URL, currentProfileId)
                       .arg(limit)
                       .arg(cursorParam);
    } else {
        if (friendsFeed) {
            endpoint = QString("%1/api/posts/feed/follow?limit=%2%3")
                           .arg(API_BASE_URL)
                           .arg(limit)
                           .arg(cursorParam);
        } else {
            endpoint = QString("%1/api/posts/feed?limit=%2%3")
                           .arg(API_BASE_URL)
                           .arg(limit)
                           .arg(cursorParam);
        }
    }
    QUrl url(endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    loadingLabel->setVisible(true);
    loadMoreButton->setVisible(false);
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onLoadPostsFinished(reply); });
}

void FeedWindow::loadMore() {
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

void FeedWindow::onAuthorClicked(const QString &authorId, bool isMePost) {
    if (isMePost) {
        loadProfile("me");
    } else {
        loadProfile(authorId);
    }
}

void FeedWindow::onPostReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        showCustomMessage(this, "Post created successfully", ":/sources/warn_happy.png");
        if (isProfileMode) {
            loadProfile(currentProfileId);
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
        showCustomMessage(this, "Failed to create post: " + errorMsg, ":/sources/warning_01.png");
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

            if (posts.isEmpty() && lastPostId.isEmpty()) {
                showNoPostsImage();
            } else {
                for (const auto &postVal : posts) {
                    QJsonObject post = postVal.toObject();
                    addPost(post);
                    lastPostId = post["id"].isString() ? post["id"].toString()
                                                       : QString::number(post["id"].toInt());
                    lastPostDate = post["createdAt"].toString();
                }
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
    json["id"] = currentProfileId;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Followed " + currentProfileLogin, ":/sources/warn_happy.png");
            loadProfile(currentProfileId);
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
    json["id"] = currentProfileId;
    QByteArray data = QJsonDocument(json).toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            showCustomMessage(this, "Unfollowed " + currentProfileLogin,
                              ":/sources/warn_happy.png");
            loadProfile(currentProfileId);
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
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.setFixedSize(360, 480);
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowMaximizeButtonHint);
    dialog.setWindowFlags(dialog.windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(10, 10, 10, 10);

    QListWidget *listWidget = new QListWidget(&dialog);
    listWidget->setStyleSheet(
        "QListWidget { border: none; background-color: transparent; outline: none; }"
        "QListWidget::item { border-bottom: 1px solid rgba(200,200,200,0.5); border-radius: 10px; }"
        "QListWidget::item:hover { background-color: rgba(200,200,200,0.2); }"
        "QListWidget::item:selected { background-color: rgba(180,180,180,0.4); color: black; }");
    layout->addWidget(listWidget);

    QPushButton *closeButton = new QPushButton("Close", &dialog);
    closeButton->setFixedSize(100, 40);
    closeButton->setStyleSheet("QPushButton { background-color: rgba(200,200,200,0.6); border: "
                               "none; border-radius: 10px; font-size: 16px; }"
                               "QPushButton:hover { background-color: rgba(180,180,180,0.8); }");
    layout->addWidget(closeButton, 0, Qt::AlignCenter);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    QUrl url(API_BASE_URL + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkAccessManager *manager = new QNetworkAccessManager(&dialog);
    QNetworkReply *reply = manager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isArray()) {
            QJsonArray users = doc.array();
            if (users.isEmpty()) {
                QListWidgetItem *item = new QListWidgetItem("No users found", listWidget);
                item->setTextAlignment(Qt::AlignCenter);
            } else {
                for (const auto &userVal : users) {
                    if (userVal.isObject()) {
                        QJsonObject user = userVal.toObject();
                        QString username = user["login"].toString();
                        QString avatarBase64 = user["avatar"].toString();
                        if (avatarBase64.isEmpty()) {
                            avatarBase64 = user["image"].toString();
                        }
                        QString id = user["id"].isString() ? user["id"].toString()
                                                           : QString::number(user["id"].toInt());

                        QListWidgetItem *item = new QListWidgetItem(listWidget);
                        item->setSizeHint(QSize(listWidget->width() - 20, 64));
                        item->setData(Qt::UserRole, id);

                        UserListItem *customWidget =
                            new UserListItem(username, avatarBase64, listWidget);
                        listWidget->setItemWidget(item, customWidget);
                    }
                }
            }
        } else {
            showCustomMessage(this, "Invalid response format", ":/sources/warning_01.png");
        }
    } else {
        showCustomMessage(this, "Failed to load " + title + ": " + reply->errorString(),
                          ":/sources/warning_01.png");
    }
    reply->deleteLater();

    connect(listWidget, &QListWidget::itemClicked, [this, &dialog](QListWidgetItem *item) {
        QString id = item->data(Qt::UserRole).toString();
        if (!id.isEmpty()) {
            dialog.accept();
            loadProfile(id);
        }
    });

    dialog.exec();
}

void FeedWindow::onFollowersClicked() {
    if (currentProfileId.isEmpty() || currentProfileId == "me") {
        showCustomMessage(this, "Cannot determine user id", ":/sources/warning_01.png");
        return;
    }

    QString endpoint = QString("/api/friends/%1/followers").arg(currentProfileId);
    showUserList("Followers", endpoint);
}

void FeedWindow::onFollowingClicked() {
    if (currentProfileId.isEmpty() || currentProfileId == "me") {
        showCustomMessage(this, "Cannot determine user id", ":/sources/warning_01.png");
        return;
    }

    QString endpoint = QString("/api/friends/%1/following").arg(currentProfileId);
    showUserList("Following", endpoint);
}

void FeedWindow::onEditProfileClicked() {
    QUrl url(API_BASE_URL + "/api/me/profile");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = this->networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
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

                connect(&dialog, &EditProfileDialog::passwordChanged, this,
                        &FeedWindow::logoutRequested);

                connect(&dialog, &EditProfileDialog::profileUpdated,
                        [this](const QString &login, const QString &email, const QString &phone,
                               bool isPrivate, const QString &avatarBase64) {
                            QUrl updateUrl(API_BASE_URL + "/api/me/profile");
                            QNetworkRequest updateRequest(updateUrl);
                            updateRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                                    "application/json");
                            updateRequest.setRawHeader("Authorization",
                                                       "Bearer " + authToken.toUtf8());

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

                            QNetworkReply *updateReply = this->networkManager->sendCustomRequest(
                                updateRequest, "PATCH", updateData);

                            connect(updateReply, &QNetworkReply::finished, [this, updateReply]() {
                                if (updateReply->error() == QNetworkReply::NoError) {
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
                                    showCustomMessage(this, "Update failed: " + errorMsg,
                                                      ":/sources/warning_01.png");
                                }
                                updateReply->deleteLater();
                            });
                        });
                dialog.exec();
            }
        } else {
            QByteArray response = reply->readAll();
            QString errorMsg = reply->errorString();
            QJsonDocument doc = QJsonDocument::fromJson(response);
            if (doc.isObject() && doc.object().contains("reason")) {
                errorMsg = doc.object()["reason"].toString();
            }
            showCustomMessage(this, "Failed to load profile data: " + errorMsg,
                              ":/sources/warning_01.png");
        }
        reply->deleteLater();
    });
}

void FeedWindow::showError(const QString &message) {
    showCustomMessage(this, message, ":/sources/warning_01.png");
}