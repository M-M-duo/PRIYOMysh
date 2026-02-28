#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QDateTime>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void checkConnection();
    void onReplyFinished(QNetworkReply *reply);
    void toggleAutoCheck();
    void onLoginClicked();
    void onRegisterClicked();
    void onAuthReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QTimer *timer;
    QLabel *statusLabel;
    QTextEdit *responseText;
    QPushButton *autoCheckButton;
    bool autoCheckEnabled = true;

    QPushButton *loginButton;
    QPushButton *registerButton;

    void setupUI();
    void updateStatus(const QString &message, const QString &color);
    void showAuthDialog(const QString &mode);
    void sendAuthRequest(const QString &login, const QString &password, const QString &mode);
};

#endif // MAINWINDOW_H