#include "mainwindow.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonParseError>
#include <QMessageBox>
#include <QScrollBar>

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
    
    QLabel *titleLabel = new QLabel("Tets-connection window", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 10px; color: #333;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
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
    
    // Область ответа
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
    responseText->setText("Waiting for the response...");
}

void MainWindow::onReplyFinished(QNetworkReply *reply) {
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        
        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response, &parseError);
        
        if (parseError.error == QJsonParseError::NoError && !jsonDoc.isNull()) {
            QString formattedJson = QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Indented));
            updateStatus("Connection success (" + timestamp + ")", "#d4edda");
            responseText->setText(formattedJson);
        } else {
            updateStatus("Connection success (" + timestamp + ", не JSON)", "#d4edda");
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
                errorMsg = "Check connection";
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
