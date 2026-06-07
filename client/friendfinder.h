// friendfinder.h
#ifndef FRIENDFINDER_H
#define FRIENDFINDER_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>

class FeedWindow;

class FriendFinder : public QDialog {
    Q_OBJECT
public:
    explicit FriendFinder(const QString &token, FeedWindow *parent = nullptr);
    ~FriendFinder();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void searchUser();
    void followUser();
    void unfollowUser();
    void onSearchFinished(QNetworkReply *reply);
    void onFollowFinished(QNetworkReply *reply);
    void onUnfollowFinished(QNetworkReply *reply);
    void onViewProfile();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    FeedWindow *parentFeedWindow;
    QString currentSearchId;
    bool isFollowing;
    bool isMutual;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QWidget *resultWidget;
    QLabel *resultLabel;
    QPushButton *actionButton;
    QLabel *statusLabel;

    void setupUI();
    void clearResult();
};

#endif