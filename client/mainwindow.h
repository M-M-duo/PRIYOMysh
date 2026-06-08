#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class QStackedWidget;
class QWidget;
class FeedWindow;
class QDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLogout();
    void onLoginClicked(const QString &login, const QString &password);
    void onRegisterClicked(const QString &login, const QString &password, const QString &email,
                           const QString &phone, bool isPrivate, const QString &avatarBase64);
    void onAuthReplyFinished(QNetworkReply *reply);

private:
    void sendAuthRequest(const QString &login, const QString &password, const QString &email,
                         const QString &phone, bool isPrivate, const QString &avatarBase64,
                         const QString &mode);
    void sendSignInRequest(const QString &login, const QString &password);

    QStackedWidget *stackedWidget;
    QWidget *loginWidget;
    FeedWindow *feedWindow;
    QDialog *currentDialog;
    QNetworkAccessManager *networkManager;

    QString pendingLogin;
    QString pendingPassword;
    QString pendingEmail;
    QString pendingPhone;
    bool pendingIsPrivate;
    QString pendingMode;
};

#endif // MAINWINDOW_H