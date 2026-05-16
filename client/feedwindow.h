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

class FriendFinder;
class PostWidget;

class FeedWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FeedWindow(const QString &token, const QString &username = QString(), QWidget *parent = nullptr);
    ~FeedWindow();

public slots:
    void onAuthorClicked(const QString &author);
    void onLikeDislike(const QString &postId, bool isLike);

private slots:
    void loadPosts(bool append = false);
    void loadMore();
    void onCreatePost();
    void onProfileClick();
    void onFindFriendsClicked();
    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onToggleFeedShared();
    void onToggleFeedFriends();

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
    QPushButton *sharedButton;
    QPushButton *friendsButton;
    QLabel *loadingLabel;
    QMap<QString, PostWidget*> m_postWidgets;

    void setupUI();
    void clearPosts();
    void addPost(const QJsonObject &post);
    void showError(const QString &message);
    void updatePostReaction(const QString &postId, int newLikes, int newDislikes);
};

#endif