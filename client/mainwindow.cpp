#include "mainwindow.h"
#include "authdialog.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonParseError>
#include <QMessageBox>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QJsonDocument>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    networkManager = new QNetworkAccessManager(this);
    timer = new QTimer(this);

    setupUI();

    connect(timer, &QTimer::timeout, this, &MainWindow::checkConnection);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::onReplyFinished);

    timer->start(5000); 
    checkConnection();  

    setWindowTitle("Ping Monitor - localhost:3000");
    resize(800, 600);
}

MainWindow::~MainWindow() {
    timer->stop();
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *topBarLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel("Test-connection window", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    topBarLayout->addWidget(titleLabel);

    topBarLayout->addStretch();

    loginButton = new QPushButton("Login", this);
    loginButton->setStyleSheet(R"(
        QPushButton {
            background-color: #007bff;
            color: white;
            padding: 6px 12px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #0056b3;
        }
    )");
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);

    registerButton = new QPushButton("Register", this);
    registerButton->setStyleSheet(R"(
        QPushButton {
            background-color: #28a745;
            color: white;
            padding: 6px 12px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #218838;
        }
    )");
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);

    topBarLayout->addWidget(loginButton);
    topBarLayout->addWidget(registerButton);

    mainLayout->addLayout(topBarLayout);

    statusLabel = new QLabel("Checking...", this);
    statusLabel->setStyleSheet("font-size: 14px; padding: 10px; border-radius: 5px; background-color: #fff3cd; color: #856404; border: 1px solid #ffeaa7;");
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *checkNowButton = new QPushButton("ping", this);
    checkNowButton->setStyleSheet(R"(
        QPushButton {
            background-color: #007bff;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #0056b3;
        }
    )");
    connect(checkNowButton, &QPushButton::clicked, this, &MainWindow::checkConnection);

    autoCheckButton = new QPushButton("stop auto-ping", this);
    autoCheckButton->setStyleSheet(R"(
        QPushButton {
            background-color: #6c757d;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #545b62;
        }
    )");
    connect(autoCheckButton, &QPushButton::clicked, this, &MainWindow::toggleAutoCheck);

    buttonLayout->addWidget(checkNowButton);
    buttonLayout->addWidget(autoCheckButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    QLabel *responseLabel = new QLabel("Response:", this);
    responseLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    mainLayout->addWidget(responseLabel);

    responseText = new QTextEdit(this);
    responseText->setReadOnly(true);
    responseText->setStyleSheet(R"(
        QTextEdit {
            font-family: 'Monospace', 'Courier New';
            font-size: 12px;
            background-color: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 10px;
        }
    )");
    mainLayout->addWidget(responseText, 1); 

    QLabel *timeLabel = new QLabel("auto check", this);
    timeLabel->setStyleSheet("color: #6c757d; font-size: 12px; padding: 5px;");
    timeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(timeLabel);

    setCentralWidget(centralWidget);
}

void MainWindow::checkConnection() {
    QUrl url("http://localhost:3000");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6 Ping Monitor");
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);
    request.setTransferTimeout(5000);

    networkManager->get(request);

    updateStatus("localhost:3000 Checking...", "#fff3cd");
    responseText->setText("Ожидание ответа от сервера...");
}

void MainWindow::onReplyFinished(QNetworkReply *reply) {
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);

        if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull()) {
            QString formattedJson = QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Indented));
            updateStatus("Connection success  (" + timestamp + ")", "#d4edda");
            responseText->setText(formattedJson);
        } else {
            updateStatus("Connection success  (" + timestamp + ", не JSON)", "#d4edda");
            responseText->setText("Server response:\n" + QString(response));
        }
    } else {
        QString errorMsg;

        switch (reply->error()) {
            case QNetworkReply::ConnectionRefusedError:
                errorMsg = "Server unavailable";
                break;
            case QNetworkReply::HostNotFoundError:
                errorMsg = "404";
                break;
            case QNetworkReply::TimeoutError:
                errorMsg = "504 Timeout";
                break;
            case QNetworkReply::NetworkSessionFailedError:
                errorMsg = "Connection error";
                break;
            default:
                errorMsg = "Error: " + reply->errorString() + "\nCode: " +
                          QString::number(reply->error());
        }

        updateStatus("No connection (" + timestamp + ")", "#f8d7da");
        responseText->setText(errorMsg + "\n\nCheck everything\n");
    }

    reply->deleteLater();
}

void MainWindow::toggleAutoCheck() {
    if (autoCheckEnabled) {
        timer->stop();
        autoCheckButton->setText("Enable auto-check");
        autoCheckButton->setStyleSheet(R"(
            QPushButton {
                background-color: #28a745;
                color: white;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #218838;
            }
        )");
    } else {
        timer->start(5000);
        autoCheckButton->setText("Stop auto-check");
        autoCheckButton->setStyleSheet(R"(
            QPushButton {
                background-color: #6c757d;
                color: white;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #545b62;
            }
        )");
    }
    autoCheckEnabled = !autoCheckEnabled;
}

void MainWindow::updateStatus(const QString &message, const QString &color) {
    QString style = QString("font-size: 14px; padding: 10px; border-radius: 5px; "
                           "background-color: %1; color: %2; border: 1px solid %3;")
                           .arg(color)
                           .arg(color == "#d4edda" ? "#155724" :
                                color == "#f8d7da" ? "#721c24" : "#856404")
                           .arg(color == "#d4edda" ? "#c3e6cb" :
                                color == "#f8d7da" ? "#f5c6cb" : "#ffeaa7");

    statusLabel->setText(message);
    statusLabel->setStyleSheet(style);
}


void MainWindow::onLoginClicked() {
    showAuthDialog("login");
}

void MainWindow::onRegisterClicked() {
    showAuthDialog("register");
}

void MainWindow::showAuthDialog(const QString &mode) {
    AuthDialog dialog(mode, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();
        if (login.isEmpty() || password.isEmpty()) {
            QMessageBox::warning(this, "Error", "Empty login/password");
            return;
        }
        sendAuthRequest(login, password, mode);
    }
}

void MainWindow::sendAuthRequest(const QString &login, const QString &password, const QString &mode) {
    QUrl url(QString("http://192.0.0.1:8000/api/%1").arg(mode));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6 Auth Client");
    request.setTransferTimeout(5000);

    QJsonObject json;
    json["login"] = login;
    json["password"] = password;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = networkManager->post(request, data);
    reply->setProperty("auth_mode", mode);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onAuthReplyFinished(reply);
    });
}

void MainWindow::onAuthReplyFinished(QNetworkReply *reply) {
    QString mode = reply->property("auth_mode").toString();
    QString title = (mode == "login") ? "login" : "register";
    QString message;
    QMessageBox::Icon icon;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        if (!jsonDoc.isNull() && jsonDoc.object().contains("message")) {
            message = jsonDoc.object()["message"].toString();
        } else {
            message = "succesful response";
        }
        icon = QMessageBox::Information;
    } else {
        message = "Error: " + reply->errorString();
        icon = QMessageBox::Critical;
    }

    QMessageBox::information(this, title, message);
    reply->deleteLater();
}