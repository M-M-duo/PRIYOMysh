#ifndef FEEDWINDOW_H
#define FEEDWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMap>

class FeedWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FeedWindow(const QString &token, const QString &username = QString(), QWidget *parent = nullptr);
    ~FeedWindow();

public slots:
    void onAuthorClicked(const QString &author);

private slots:
    void loadPosts(bool append = false);
    void loadMore();
    void onCreatePost();
    void onProfileClick();
    void onFindFriendsClicked();
    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onToggleFeedType();
    void onLikeDislike(const QString &postId, bool isLike);

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentUsername;
    int currentOffset;
    int limit;
    bool friendsFeed;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *postsLayout;
    QPushButton *loadMoreButton;
    QPushButton *createPostButton;
    QPushButton *profileButton;
    QPushButton *findFriendsButton;
    QPushButton *toggleFeedButton;
    QLabel *loadingLabel;
    QMap<QString, class PostWidget*> m_postWidgets;

    void setupUI();
    void clearPosts();
    void addPost(const QJsonObject &post);
    void showError(const QString &message);
    void updatePostReaction(const QString &postId, int newLikes, int newDislikes);
};

#endif