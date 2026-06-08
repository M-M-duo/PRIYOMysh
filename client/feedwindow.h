#ifndef FEEDWINDOW_H
#define FEEDWINDOW_H

#include <QJsonObject>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

class PostWidget;
class FriendFinder;
class QNetworkReply;

class FeedWindow : public QMainWindow {
    Q_OBJECT
    friend class PostWidget;
    friend class FriendFinder;

public:
    explicit FeedWindow(const QString &token, QWidget *parent = nullptr);
    ~FeedWindow();

    void loadProfile(const QString &id);
    void onLikeDislike(const QString &postId, bool isLike);
    void onAuthorClicked(const QString &authorId, bool isMePost);

signals:
    void logoutRequested();

private slots:
    void onCreatePost();
    void onProfileClick();
    void onFindFriendsClicked();
    void onBackClicked();
    void onToggleFeedShared();
    void onToggleFeedFollow();
    void loadMore();
    void onEditProfileClicked();
    void onFollowFromProfile();
    void onUnfollowFromProfile();
    void onFollowersClicked();
    void onFollowingClicked();

    void onPostReplyFinished(QNetworkReply *reply);
    void onLoadPostsFinished(QNetworkReply *reply);
    void onProfileInfoFinished(QNetworkReply *reply);

private:
    void setupUI();
    void loadFeed(bool friendsOnly);
    void loadPosts(bool append);
    void loadMyProfile();
    void updateProfileHeader(const QJsonObject &profile);
    void clearPosts();
    void addPost(const QJsonObject &post);
    void showFeedButtons();
    void showProfileButtons();
    void resetToMainFeed();
    void updatePostReaction(const QString &postId, int newLikes, int newDislikes);
    void showUserList(const QString &title, const QString &endpoint);
    void showNoPostsImage();
    void showError(const QString &message);

    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentProfileId;
    QString currentProfileLogin;

    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    QVBoxLayout *postsLayout;
    QWidget *profileHeader;

    QPushButton *backButton;
    QPushButton *exitButton;
    QPushButton *findFriendsButton;
    QPushButton *createPostButton;
    QPushButton *profileButton;
    QPushButton *sharedButton;
    QPushButton *followButton;
    QPushButton *loadMoreButton;
    QPushButton *followProfileButton;
    QPushButton *followersButton;
    QPushButton *followingButton;
    QPushButton *postsButton;

    QLabel *avatarLabel;
    QLabel *profileLoginLabel;
    QLabel *loadingLabel;

    QMap<QString, PostWidget *> m_postWidgets;
    QString lastPostId;
    QString lastPostDate;
    int limit;
    bool friendsFeed;
    bool isProfileMode;
};

#endif // FEEDWINDOW_H