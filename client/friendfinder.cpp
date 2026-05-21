#include "friendfinder.h"
#include "feedwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QScreen>
#include <QUrl>
#include <QVBoxLayout>

const QString API_BASE_URL = "http://127.0.0.1:8080";

static void showMessage(QWidget *parent, const QString &text, QMessageBox::Icon icon) {
    QMessageBox msgBox(parent);
    msgBox.setIcon(icon);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    msgBox.move(170, (screenHeight - msgBox.height()) / 2);
    msgBox.exec();
}

static QString extractErrorMessage(QNetworkReply *reply) {
    QByteArray response = reply->readAll();
    QString errorMsg = reply->errorString();
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isObject() && doc.object().contains("reason")) {
        errorMsg = doc.object()["reason"].toString();
    }
    return errorMsg;
}

FriendFinder::FriendFinder(const QString &token, QWidget *parent)
    : QDialog(parent), authToken(token) {
    networkManager = new QNetworkAccessManager(this);
    setupUI();
}

FriendFinder::~FriendFinder() {}

void FriendFinder::setupUI() {
    setWindowTitle("PRIYOMYSH");
    resize(400, 150);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Enter login...");
    searchButton = new QPushButton("Search", this);
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    resultLabel = new QLabel("", this);
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setCursor(Qt::PointingHandCursor);
    resultLabel->installEventFilter(this);
    resultLabel->setVisible(false);
    layout->addWidget(resultLabel);

    connect(searchButton, &QPushButton::clicked, this, &FriendFinder::searchUser);
}

void FriendFinder::clearResult() {
    resultLabel->setVisible(false);
    resultLabel->clear();
}

void FriendFinder::searchUser() {
    QString login = searchEdit->text().trimmed();
    if (login.isEmpty()) {
        showMessage(this, "Enter login", QMessageBox::Critical);
        return;
    }
    clearResult();
    currentSearchLogin = login;

    QUrl url(QString("%1/api/friends/search/%2").arg(API_BASE_URL, login));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onSearchFinished(reply); });
}

void FriendFinder::onSearchFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString login = obj["login"].toString();
            currentSearchLogin = login;
            resultLabel->setText(login);
            resultLabel->setVisible(true);
        } else {
            resultLabel->setText("User not found");
            resultLabel->setVisible(true);
        }
    } else {
        resultLabel->setText("Search failed");
        resultLabel->setVisible(true);
    }
    reply->deleteLater();
}

void FriendFinder::onViewProfile() {
    if (!currentSearchLogin.isEmpty()) {
        FeedWindow *userFeed = new FeedWindow(authToken, currentSearchLogin);
        userFeed->show();
        close();
    }
}

bool FriendFinder::eventFilter(QObject *obj, QEvent *event) {
    if (obj == resultLabel && event->type() == QEvent::MouseButtonPress) {
        onViewProfile();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}