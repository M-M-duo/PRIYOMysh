#ifndef FRIENDFINDER_H
#define FRIENDFINDER_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class FriendFinder : public QDialog {
    Q_OBJECT
public:
    explicit FriendFinder(const QString &token, QWidget *parent = nullptr);
    ~FriendFinder();

private slots:
    void searchUser();
    void followUser();
    void unfollowUser();
    void onSearchFinished(QNetworkReply *reply);
    void onFollowFinished(QNetworkReply *reply);
    void onUnfollowFinished(QNetworkReply *reply);
    void onViewPosts();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentSearchLogin;
    bool isFollowing;
    bool isMutual;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QWidget *resultWidget;
    QLabel *resultLabel;
    QPushButton *viewPostsButton;
    QPushButton *actionButton;
    QLabel *statusLabel;

    void setupUI();
    void showError(const QString &message);
    void clearResult();
};

#endif