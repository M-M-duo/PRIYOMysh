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
    explicit FeedWindow(const QString &token, QWidget *parent = nullptr);
    ~FeedWindow();

    void loadFeed(bool friendsOnly = false);
    void loadProfile(const QString &id);
    void loadMyProfile();

public slots:
    void onAuthorClicked(const QString &authorId, bool isMePost);
    void onLikeDislike(const QString &postId, bool isLike);

private slots:
    void loadPosts(bool append = false);
    void loadMore();
    void onCreatePost();
    void onProfileClick();
    void onFindFriendsClicked();
    void onBackClicked();
    void onToggleFeedShared();
    void onToggleFeedFollow();
    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onProfileInfoFinished(QNetworkReply *reply);
    void onFollowFromProfile();
    void onUnfollowFromProfile();
    void onFollowersClicked();
    void onFollowingClicked();
    void onEditProfileClicked();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString myActualId;
    QString myActualLogin;
    QString lastPostDate;
    QString lastPostId;
    int limit;
    bool friendsFeed;
    bool isOwnProfile;
    bool isProfileMode;
    QString currentProfileId;
    QString currentProfileLogin;

    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *postsLayout;
    QPushButton *loadMoreButton;
    QPushButton *createPostButton;
    QPushButton *profileButton;
    QPushButton *findFriendsButton;
    QPushButton *backButton;
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
    void updateProfileHeader(const QJsonObject &profile);
    void showNoPostsImage();
    void showUserList(const QString &title, const QString &endpoint);
    void resetToMainFeed();
    void showFeedButtons();
    void showProfileButtons();
};

#endif