#ifndef FRIENDFINDER_H
#define FRIENDFINDER_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>

class FriendFinder : public QDialog {
    Q_OBJECT
public:
    explicit FriendFinder(const QString &token, QWidget *parent = nullptr);
    ~FriendFinder();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void searchUser();
    void onSearchFinished(QNetworkReply *reply);
    void onViewProfile();

private:
    QNetworkAccessManager *networkManager;
    QString authToken;
    QString currentSearchLogin;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QLabel *resultLabel;

    void setupUI();
    void clearResult();
};

#endif