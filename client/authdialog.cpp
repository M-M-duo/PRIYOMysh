#include "authdialog.h"
#include <QBuffer>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

static QString formatPhoneNumber(const QString &raw) {
    QString digits;
    for (QChar ch : raw) {
        if (ch.isDigit())
            digits.append(ch);
    }
    QString result;
    if (digits.length() >= 0)
        result = "+7";
    if (digits.length() >= 1)
        result += digits.mid(1, 10);
    return result;
}

AuthDialog::AuthDialog(const QString &mode, QWidget *parent) : QDialog(parent), mode(mode) {
    setupUI();
}

void AuthDialog::setupUI() {
    setWindowTitle("PRIYOMYSH");
    if (mode == "login") {
        setFixedSize(360, 200);
    } else {
        setFixedSize(360, 480);
    }
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    if (mode == "login") {
        this->move(45, 360);
    } else {
        this->move(45, 180);
    }

    if (mode == "register") {
        QHBoxLayout *avatarLayout = new QHBoxLayout();
        avatarLayout->addStretch();
        avatarLabel = new QLabel(this);
        avatarLabel->setFixedSize(64, 64);
        avatarLabel->setAlignment(Qt::AlignCenter);
        avatarLabel->setCursor(Qt::PointingHandCursor);
        avatarLabel->installEventFilter(this);

        QPixmap defaultPix(":/sources/default_ava.png");
        if (!defaultPix.isNull()) {
            QPixmap scaled =
                defaultPix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap rounded(64, 64);
            rounded.fill(Qt::transparent);

            QPainter painter(&rounded);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(QBrush(scaled));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(0, 0, 64, 64, 32, 32);

            avatarLabel->setPixmap(rounded);
            avatarLabel->setStyleSheet("border: none;");
        } else {
            avatarLabel->setText("🖼️");
            avatarLabel->setStyleSheet(
                "border: 1px solid #ccc; border-radius: 32px; background-color: #e0e0e0;");
        }

        avatarLayout->addWidget(avatarLabel);
        avatarLayout->addStretch();
        layout->addLayout(avatarLayout);
    }

    layout->addWidget(new QLabel("Login:"));
    loginEdit = new QLineEdit(this);
    loginEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                             "border-radius: 10px; padding: 8px;");
    layout->addWidget(loginEdit);

    layout->addWidget(new QLabel("Password:"));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                                "border-radius: 10px; padding: 8px;");
    layout->addWidget(passwordEdit);

    if (mode == "register") {
        layout->addWidget(new QLabel("Email:"));
        emailEdit = new QLineEdit(this);
        emailEdit->setPlaceholderText("example@mail.ru");
        emailEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                                 "border-radius: 10px; padding: 8px;");
        layout->addWidget(emailEdit);

        layout->addWidget(new QLabel("Phone:"));
        phoneEdit = new QLineEdit(this);
        phoneEdit->setPlaceholderText("+7 910 294 10 01");
        phoneEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: none; "
                                 "border-radius: 10px; padding: 8px;");

        connect(phoneEdit, &QLineEdit::textChanged, [this](const QString &text) {
            if (text.isEmpty())
                return;
            QString formatted = formatPhoneNumber(text);
            if (formatted != text) {
                int cursorPos = phoneEdit->cursorPosition();
                phoneEdit->blockSignals(true);
                phoneEdit->setText(formatted);
                phoneEdit->setCursorPosition(qMin(cursorPos, formatted.length()));
                phoneEdit->blockSignals(false);
            }
        });

        phoneEdit->installEventFilter(this);
        layout->addWidget(phoneEdit);

        privateCheckBox = new QCheckBox("Private profile", this);
        privateCheckBox->setChecked(false);
        layout->addWidget(privateCheckBox);
    }

    layout->addSpacing(10);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox, 0, Qt::AlignCenter);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    QPushButton *cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    if (okButton) {
        okButton->setFixedSize(100, 40);
        okButton->setStyleSheet("QPushButton { background-color: rgba(200,200,200,0.6); border: "
                                "none; border-radius: 10px; font-size: 16px; }"
                                "QPushButton:hover { background-color: rgba(180,180,180,0.8); }");
    }
    if (cancelButton) {
        cancelButton->setFixedSize(100, 40);
        cancelButton->setStyleSheet(
            "QPushButton { background-color: rgba(200,200,200,0.6); border: none; border-radius: "
            "10px; font-size: 16px; }"
            "QPushButton:hover { background-color: rgba(180,180,180,0.8); }");
    }

    connect(buttonBox, &QDialogButtonBox::accepted, this, &AuthDialog::onButtonClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AuthDialog::enableButtons(bool enabled) {
    QDialogButtonBox *buttonBox = findChild<QDialogButtonBox *>();
    if (buttonBox) {
        buttonBox->setEnabled(enabled);
    }
}

bool AuthDialog::eventFilter(QObject *obj, QEvent *event) {
    if (mode == "register" && obj == phoneEdit && event->type() == QEvent::FocusIn) {
        if (phoneEdit->text().isEmpty()) {
            phoneEdit->setText("+7");
            phoneEdit->setCursorPosition(2);
        }
    }
    if (mode == "register" && obj == avatarLabel && event->type() == QEvent::MouseButtonPress) {
        chooseAvatar();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

void AuthDialog::chooseAvatar() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Avatar", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty())
        return;

    QString base64 = cropAndToBase64(filePath);
    if (base64.isEmpty()) {
        QMessageBox::warning(this, "Error", "Failed to load image");
        return;
    }
    avatarBase64 = base64;
    QPixmap pixmap;
    pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
    if (!pixmap.isNull()) {
        QPixmap scaled = pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmap rounded(64, 64);
        rounded.fill(Qt::transparent);

        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QBrush(scaled));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 64, 64, 32, 32);

        avatarLabel->setPixmap(rounded);
        avatarLabel->setStyleSheet("border: none;");
    }
}

QString AuthDialog::cropAndToBase64(const QString &filePath) {
    QImage image(filePath);
    if (image.isNull())
        return QString();

    int size = qMin(image.width(), image.height());
    int x = (image.width() - size) / 2;
    int y = (image.height() - size) / 2;
    QImage cropped = image.copy(x, y, size, size);

    const int maxSize = 64;
    if (cropped.width() > maxSize || cropped.height() > maxSize) {
        cropped = cropped.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    cropped.save(&buffer, "JPEG", 85);
    buffer.close();

    return QString::fromLatin1(byteArray.toBase64());
}

void AuthDialog::onButtonClicked() {
    // Убрали отключение кнопок, чтобы они оставались кликабельными
    if (mode == "login") {
        emit loginClicked(getLogin(), getPassword());
    } else {
        emit registerClicked(getLogin(), getPassword(), getEmail(), getPhone(), isPrivate(),
                             getAvatarBase64());
    }
}

QString AuthDialog::getLogin() const {
    return loginEdit->text();
}
QString AuthDialog::getPassword() const {
    return passwordEdit->text();
}
QString AuthDialog::getEmail() const {
    return emailEdit ? emailEdit->text() : QString();
}
QString AuthDialog::getPhone() const {
    return phoneEdit ? phoneEdit->text() : QString();
}
bool AuthDialog::isPrivate() const {
    return privateCheckBox ? privateCheckBox->isChecked() : false;
}
QString AuthDialog::getAvatarBase64() const {
    return avatarBase64;
}

void AuthDialog::clearField(const QString &fieldName) {
    if (fieldName == "login")
        loginEdit->clear();
    else if (fieldName == "password")
        passwordEdit->clear();
    else if (fieldName == "email" && emailEdit)
        emailEdit->clear();
    else if (fieldName == "phone" && phoneEdit)
        phoneEdit->clear();
}