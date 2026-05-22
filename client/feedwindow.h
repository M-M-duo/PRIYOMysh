#ifndef FEEDWINDOW_H
#define FEEDWINDOW_H

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

class FriendFinder;
class PostWidget;

class FeedWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit FeedWindow(const QString &token, const QString &username = QString(),
                        QWidget *parent = nullptr);
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
    void onExitProfile();
    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onToggleFeedShared();
    void onToggleFeedFollow();
    void onProfileInfoFinished(QNetworkReply *reply);
    void onFollowFromProfile();
    void onUnfollowFromProfile();
    void onMyLoginFinished(QNetworkReply *reply);
    void onFollowersClicked();
    void onFollowingClicked();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentUsername;
    QString myActualLogin;
    int currentOffset;
    int limit;
    bool followFeed;
    bool isOwnProfile;
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *postsLayout;
    QPushButton *loadMoreButton;
    QPushButton *createPostButton;
    QPushButton *profileButton;
    QPushButton *findFriendsButton;
    QPushButton *exitProfileButton;
    QPushButton *sharedButton;
    QPushButton *followButton;
    QPushButton *followersButton;
    QPushButton *followingButton;
    QPushButton *postsButton;
    QPushButton *followProfileButton;
    QLabel *loadingLabel;
    QMap<QString, PostWidget *> m_postWidgets;

    QWidget *profileHeader;
    QLabel *profileLoginLabel;
    QLabel *avatarLabel;

    void setupUI();
    void clearPosts();
    void addPost(const QJsonObject &post);
    void showError(const QString &message);
    void updatePostReaction(const QString &postId, int newLikes, int newDislikes);
    void loadProfileInfo();
    void updateProfileHeader(const QJsonObject &profile);
    void showNoPostsImage();
    void fetchMyLogin();
    void showUserList(const QString &title, const QString &endpoint);
};

#endif