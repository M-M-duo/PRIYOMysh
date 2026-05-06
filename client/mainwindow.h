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
    void onSignInClicked();
    void onRegisterClicked();
    void onAuthReplyFinished(QNetworkReply *reply);
    void onSignInReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    void showAuthDialog(const QString &mode);
    void showSignInDialog(const QString &mode);
    void sendAuthRequest(const QString &login, const QString &password,
                         const QString &email, const QString &phone, bool isPublic, const QString &mode);
    void sendSignInRequest(const QString &login, const QString &password);
};

#endif