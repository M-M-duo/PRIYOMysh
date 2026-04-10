#include "feedwindow.h"
#include "postdialog.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QDateTime>
#include <QDebug>
#include <QFrame>
#include <QPixmap>
#include <QLabel>
#include <QEvent>

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

class PostWidget : public QWidget {
public:
    PostWidget(const QJsonObject &post, FeedWindow *parent = nullptr) : QWidget(parent), feedWindow(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(5);
        layout->setContentsMargins(5, 5, 5, 5);

        authorLabel = new QLabel(post["author"].toString());
        authorLabel->setStyleSheet("font-weight: bold; color: #007bff; text-decoration: underline;");
        authorLabel->setCursor(Qt::PointingHandCursor);
        authorLabel->installEventFilter(this);
        layout->addWidget(authorLabel);

        // Изображение (если есть)
        if (post.contains("img") && post["img"].isArray()) {
            QJsonArray images = post["img"].toArray();
            if (!images.isEmpty()) {
                QString imgBase64 = images[0].toString();
                if (!imgBase64.isEmpty()) {
                    QPixmap pixmap;
                    pixmap.loadFromData(QByteArray::fromBase64(imgBase64.toLatin1()));
                    if (!pixmap.isNull()) {
                        QLabel *imageLabel = new QLabel(this);
                        imageLabel->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        imageLabel->setAlignment(Qt::AlignCenter);
                        layout->addWidget(imageLabel);
                    }
                }
            }
        }

        QLabel *contentLabel = new QLabel(post["content"].toString());
        contentLabel->setWordWrap(true);
        layout->addWidget(contentLabel);

        QString tagsStr;
        if (post.contains("tags") && post["tags"].isArray()) {
            QJsonArray tagsArr = post["tags"].toArray();
            for (const auto &tag : tagsArr) {
                if (!tagsStr.isEmpty()) tagsStr += " ";
                tagsStr += "#" + tag.toString();
            }
        }
        QLabel *tagsLabel = new QLabel(tagsStr);
        tagsLabel->setStyleSheet("color: #007bff;");
        layout->addWidget(tagsLabel);

        QString createdAt = post["createdAt"].toString();
        QDateTime dt = QDateTime::fromString(createdAt, "yyyy-MM-dd HH:mm:ss.zzz");
        if (!dt.isValid()) dt = QDateTime::fromString(createdAt, Qt::ISODate);
        QLabel *dateLabel = new QLabel("Posted: " + dt.toString("dd.MM.yyyy HH:mm"));
        dateLabel->setStyleSheet("color: #6c757d;");
        layout->addWidget(dateLabel);

        QHBoxLayout *likesLayout = new QHBoxLayout();
        QLabel *likesLabel = new QLabel("👍 " + QString::number(post["likesCount"].toInt()));
        QLabel *dislikesLabel = new QLabel("👎 " + QString::number(post["dislikesCount"].toInt()));
        likesLayout->addWidget(likesLabel);
        likesLayout->addWidget(dislikesLabel);
        likesLayout->addStretch();
        layout->addLayout(likesLayout);

        author = post["author"].toString();
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

private:
    QString author;
    FeedWindow *feedWindow;
    QLabel *authorLabel;
};

FeedWindow::FeedWindow(const QString &token, const QString &username, QWidget *parent)
    : QMainWindow(parent), authToken(token), currentUsername(username), currentOffset(0), limit(10)
{
    networkManager = new QNetworkAccessManager(this);
    setupUI();
    loadPosts(false);
}

FeedWindow::~FeedWindow() {}

void FeedWindow::setupUI() {
    if (currentUsername == "me")
        setWindowTitle("My Posts");
    else if (!currentUsername.isEmpty() && currentUsername != "me")
        setWindowTitle("Posts of " + currentUsername);
    else
        setWindowTitle("Feed");
    resize(600, 900);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QHBoxLayout *topBar = new QHBoxLayout();
    if (currentUsername.isEmpty()) {
        createPostButton = new QPushButton("Create Post", this);
        profileButton = new QPushButton("👤 My Profile", this);
        topBar->addWidget(createPostButton);
        topBar->addStretch();
        topBar->addWidget(profileButton);
        connect(createPostButton, &QPushButton::clicked, this, &FeedWindow::onCreatePost);
        connect(profileButton, &QPushButton::clicked, this, &FeedWindow::onProfileClick);
    } else {
        QPushButton *backButton = new QPushButton("← Back to Feed", this);
        topBar->addWidget(backButton);
        connect(backButton, &QPushButton::clicked, this, &FeedWindow::close);
    }
    mainLayout->addLayout(topBar);

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
    loadMoreButton->setVisible(false);
    mainLayout->addWidget(loadMoreButton);

    loadingLabel = new QLabel("Loading...", this);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setVisible(false);
    mainLayout->addWidget(loadingLabel);

    setCentralWidget(central);

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
    PostWidget *widget = new PostWidget(post, this);
    postsLayout->addWidget(widget);
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    postsLayout->addWidget(line);
}

void FeedWindow::loadPosts(bool append) {
    if (!append) {
        currentOffset = 0;
        clearPosts();
    }
    QString endpoint;
    if (currentUsername == "me") {
        endpoint = QString("http://127.0.0.1:8080/api/posts/feed/my?limit=%1&offset=%2").arg(limit).arg(currentOffset);
    } else if (!currentUsername.isEmpty() && currentUsername != "me") {
        endpoint = QString("http://127.0.0.1:8080/api/posts/feed/%1?limit=%2&offset=%3").arg(currentUsername).arg(limit).arg(currentOffset);
    } else {
        endpoint = QString("http://127.0.0.1:8080/api/posts/feed?limit=%1&offset=%2").arg(limit).arg(currentOffset);
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
        QStringList images = dialog.getImagesBase64();
        if (description.isEmpty() && images.isEmpty()) {
            showCustomWarning(this, "At least description or image is required");
            return;
        }
        QUrl url("http://127.0.0.1:8080/api/posts/new");
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
        connect(reply, &QNetworkReply::finished, [this, reply]() {
            onPostReplyFinished(reply);
        });
    }
}

void FeedWindow::onProfileClick() {
    FeedWindow *myPostsWindow = new FeedWindow(authToken, "me", this);
    myPostsWindow->show();
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
        showCustomInfo(this, "Post created successfully");
        loadPosts(false);
    } else {
        qDebug().noquote() << "===server error=> " << reply->errorString();
        showCustomError(this, "Failed to create post: " + reply->errorString());
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
            showCustomError(this, "Unexpected response format");
        }
    } else {
        qDebug().noquote() << "===server error=> " << reply->errorString();
        showCustomError(this, "Failed to load feed: " + reply->errorString());
        loadMoreButton->setVisible(false);
    }
    reply->deleteLater();
}

void FeedWindow::showError(const QString &message) {
    showCustomError(this, message);
}