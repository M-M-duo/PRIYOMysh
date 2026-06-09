#include "mainwindow.h"
#include "authdialog.h"
#include "feedwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

static void showCustomWarning(QWidget *parent, const QString &text) {
    QDialog dialog(parent);
    dialog.setWindowTitle("PRIYOMYSH");
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *iconLabel = new QLabel();
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(300, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconLabel->setPixmap(scaled);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel *textLabel = new QLabel(text);
    textLabel->setStyleSheet("color: white; font-size: 16px;");
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: "
                         "8px; padding: 8px 20px; font-size: 14px; }"
                         "QPushButton:hover { background-color: #0056b3; }");
    layout->addWidget(okBtn, 0, Qt::AlignCenter);

    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

static void showCustomError(QWidget *parent, const QString &text) {
    QDialog dialog(parent);
    dialog.setWindowTitle("PRIYOMYSH");
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    QLabel *iconLabel = new QLabel();
    QPixmap original(":/sources/warning_01.png");
    QPixmap scaled = original.scaled(300, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    iconLabel->setPixmap(scaled);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel *textLabel = new QLabel(text);
    textLabel->setStyleSheet("color: white; font-size: 16px;");
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setStyleSheet("QPushButton { background-color: #007bff; color: white; border-radius: "
                         "8px; padding: 8px 20px; font-size: 14px; }"
                         "QPushButton:hover { background-color: #0056b3; }");
    layout->addWidget(okBtn, 0, Qt::AlignCenter);

    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}
static void showCustomInfo(QWidget *parent, const QString &text) {
    QMessageBox msgBox(parent);
    QPixmap original(":/sources/warn_happy.png");
    QPixmap scaled = original.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    msgBox.setIconPixmap(scaled);
    msgBox.setWindowTitle("PRIYOMYSH");
    msgBox.setText(text);
    QScreen *screen = QGuiApplication::primaryScreen();
    int screenHeight = screen->availableGeometry().height();
    msgBox.move(170, (screenHeight - msgBox.height()) / 2);
    msgBox.exec();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentDialog(nullptr), feedWindow(nullptr) {
    networkManager = new QNetworkAccessManager(this);
    setWindowTitle("PRIYOMYSH");
    setFixedSize(450, 840);
    // move(0, 0);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    loginWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(loginWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *topWidget = new QWidget(loginWidget);
    topWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *topLayout = new QVBoxLayout(topWidget);
    topLayout->setAlignment(Qt::AlignCenter);

    QLabel *logoLabel = new QLabel(topWidget);
    logoLabel->setFixedSize(380, 509);
    logoLabel->setScaledContents(true);
    QPixmap logoPixmap(":/sources/enter_logo.png");
    if (!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap);
    } else {
        logoLabel->setText("Logo");
    }
    logoLabel->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(logoLabel);
    mainLayout->addWidget(topWidget, 1);

    QWidget *bottomWidget = new QWidget(loginWidget);
    QVBoxLayout *bottomLayout = new QVBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(15, 0, 15, 10);
    bottomLayout->setSpacing(10);

    QPushButton *loginBtn = new QPushButton("Login", bottomWidget);
    QPushButton *registerBtn = new QPushButton("Register", bottomWidget);

    loginBtn->setFixedHeight(48);
    registerBtn->setFixedHeight(48);
    loginBtn->setFixedWidth(420);
    registerBtn->setFixedWidth(420);

    bottomLayout->addStretch();
    bottomLayout->addWidget(loginBtn);
    bottomLayout->addWidget(registerBtn);

    mainLayout->addWidget(bottomWidget);

    stackedWidget->addWidget(loginWidget);

    QString buttonStyle = R"(
        QPushButton {
            background-color: rgba(200, 200, 200, 0.6);
            border: none;
            border-radius: 18px;
            font-size: 16px;
            font-weight: 500;
            color: #333;
        }
        QPushButton:hover {
            background-color: rgba(180, 180, 180, 0.8);
        }
        QPushButton:pressed {
            background-color: rgba(160, 160, 160, 0.9);
        }
    )";
    loginBtn->setStyleSheet(buttonStyle);
    registerBtn->setStyleSheet(buttonStyle);

    connect(loginBtn, &QPushButton::clicked, [this]() {
        if (currentDialog) {
            currentDialog->deleteLater();
        }

        AuthDialog *dialog = new AuthDialog("login", this);
        currentDialog = dialog;
        connect(dialog, &AuthDialog::loginClicked, this, &MainWindow::onLoginClicked);
        dialog->show();
    });

    connect(registerBtn, &QPushButton::clicked, [this]() {
        if (currentDialog) {
            currentDialog->deleteLater();
        }

        AuthDialog *dialog = new AuthDialog("register", this);
        currentDialog = dialog;
        connect(dialog, &AuthDialog::registerClicked, this, &MainWindow::onRegisterClicked);
        dialog->show();
    });
}

MainWindow::~MainWindow() {}

void MainWindow::onLogout() {
    if (feedWindow) {
        stackedWidget->removeWidget(feedWindow);
        feedWindow->deleteLater();
        feedWindow = nullptr;
    }
    stackedWidget->setCurrentWidget(loginWidget);
}

void MainWindow::onLoginClicked(const QString &login, const QString &password) {
    pendingLogin = login;
    pendingPassword = password;
    pendingMode = "login";
    sendSignInRequest(login, password);
}

void MainWindow::onRegisterClicked(const QString &login, const QString &password,
                                   const QString &email, const QString &phone, bool isPrivate,
                                   const QString &avatarBase64) {
    pendingLogin = login;
    pendingPassword = password;
    pendingEmail = email;
    pendingPhone = phone;
    pendingIsPrivate = isPrivate;
    pendingMode = "register";
    sendAuthRequest(login, password, email, phone, isPrivate, avatarBase64, "register");
}

void MainWindow::sendAuthRequest(const QString &login, const QString &password,
                                 const QString &email, const QString &phone, bool isPrivate,
                                 const QString &avatarBase64, const QString &mode) {
    QUrl url(QString("http://127.0.0.1:8080/api/auth/%1").arg(mode));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;
    json["email"] = email;
    json["phone"] = phone;
    json["isPublic"] = !isPrivate;
    json["image"] = avatarBase64;

    QByteArray data = QJsonDocument(json).toJson();

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << QString::fromUtf8(data);
    qDebug().noquote() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onAuthReplyFinished(reply); });
}

void MainWindow::sendSignInRequest(const QString &login, const QString &password) {
    QUrl url("http://127.0.0.1:8080/api/auth/sign-in");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;

    QByteArray data = QJsonDocument(json).toJson();

    qDebug().noquote() << "===client=> " << url.toString();
    qDebug().noquote() << QString::fromUtf8(data);
    qDebug().noquote() << "";

    QNetworkReply *reply = networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply]() { onAuthReplyFinished(reply); });
}

void MainWindow::onAuthReplyFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        qDebug().noquote() << "===server=> ";
        qDebug().noquote() << QString::fromUtf8(response);
        qDebug().noquote() << "";
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("token")) {
                QString token = obj["token"].toString();

                if (feedWindow) {
                    stackedWidget->removeWidget(feedWindow);
                    feedWindow->deleteLater();
                }

                feedWindow = new FeedWindow(token, this);
                stackedWidget->addWidget(feedWindow);
                stackedWidget->setCurrentWidget(feedWindow);

                connect(feedWindow, &FeedWindow::logoutRequested, this, &MainWindow::onLogout);

                if (currentDialog) {
                    currentDialog->accept();
                    currentDialog->deleteLater();
                    currentDialog = nullptr;
                }
            } else {
                showCustomInfo(this, "Registration successful, please sign in");
                if (currentDialog) {
                    currentDialog->reject();
                    currentDialog->deleteLater();
                    currentDialog = nullptr;
                }
            }
        }
    } else {
        QByteArray response = reply->readAll();
        QString errorMsg = reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isObject() && doc.object().contains("reason")) {
            errorMsg = doc.object()["reason"].toString();
        }
        qDebug().noquote() << "===server error=> " << errorMsg;

        if (currentDialog) {
            showCustomError(currentDialog, errorMsg);
        } else {
            showCustomError(this, errorMsg);
        }
    }
    reply->deleteLater();
}