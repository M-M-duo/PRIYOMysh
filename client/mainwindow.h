#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked(const QString &login, const QString &password);
    void onRegisterClicked(const QString &login, const QString &password,
                           const QString &email, const QString &phone, bool isPrivate, const QString &avatarBase64);
    void onAuthReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QDialog *currentDialog;
    QString pendingMode;
    QString pendingLogin;
    QString pendingPassword;
    QString pendingEmail;
    QString pendingPhone;
    bool pendingIsPrivate;

    void sendAuthRequest(const QString &login, const QString &password,
                         const QString &email, const QString &phone, bool isPrivate, const QString &avatarBase64, const QString &mode);
    void sendSignInRequest(const QString &login, const QString &password);
};

#endif