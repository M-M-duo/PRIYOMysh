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
    void onFriendsListClicked();
    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onToggleFeedShared();
    void onToggleFeedFriends();
    void onProfileInfoFinished(QNetworkReply *reply);
    void onFollowFromProfile();
    void onUnfollowFromProfile();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentUsername;
    QString profileLogin;
    int currentOffset;
    int limit;
    bool friendsFeed;
    bool isOwnProfile;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *postsLayout;
    QPushButton *loadMoreButton;
    QPushButton *createPostButton;
    QPushButton *profileButton;
    QPushButton *findFriendsButton;
    QPushButton *friendsListButton;
    QPushButton *sharedButton;
    QPushButton *friendsButton;
    QLabel *loadingLabel;
    QMap<QString, PostWidget*> m_postWidgets;

    QWidget *profileHeader;
    QLabel *profileNameLabel;
    QLabel *profileLoginLabel;
    QLabel *profileEmailLabel;
    QLabel *profilePhoneLabel;
    QLabel *followersLabel;
    QLabel *followingLabel;
    QPushButton *followProfileButton;

    void setupUI();
    void clearPosts();
    void addPost(const QJsonObject &post);
    void showError(const QString &message);
    void updatePostReaction(const QString &postId, int newLikes, int newDislikes);
    void loadProfileInfo();
    void updateProfileHeader(const QJsonObject &user);
};

#endif