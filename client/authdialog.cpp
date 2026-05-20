#include "authdialog.h"
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
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

AuthDialog::AuthDialog(const QString &mode, QWidget *parent)
    : QDialog(parent), mode(mode) {
  setupUI();
}

void AuthDialog::setupUI() {
  setWindowTitle("PRIYOMYSH");
  if (mode == "login") {
    setFixedSize(220, 200);
  } else {
    setFixedSize(220, 360);
  }
  setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
  setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
  setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setAlignment(Qt::AlignCenter);

  layout->addWidget(new QLabel("Login:"));
  loginEdit = new QLineEdit(this);
  loginEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: "
                           "none; border-radius: 10px; padding: 8px;");
  layout->addWidget(loginEdit);

  layout->addWidget(new QLabel("Password:"));
  passwordEdit = new QLineEdit(this);
  passwordEdit->setEchoMode(QLineEdit::Password);
  passwordEdit->setStyleSheet(
      "background-color: rgba(200,200,200,0.1); border: none; border-radius: "
      "10px; padding: 8px;");
  layout->addWidget(passwordEdit);

  if (mode == "register") {
    layout->addWidget(new QLabel("Email:"));
    emailEdit = new QLineEdit(this);
    emailEdit->setPlaceholderText("example@mail.ru");
    emailEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: "
                             "none; border-radius: 10px; padding: 8px;");
    layout->addWidget(emailEdit);

    layout->addWidget(new QLabel("Phone:"));
    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("+7 910 294 10 01");
    phoneEdit->setStyleSheet("background-color: rgba(200,200,200,0.1); border: "
                             "none; border-radius: 10px; padding: 8px;");

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

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  layout->addWidget(buttonBox, 0, Qt::AlignCenter);

  connect(buttonBox, &QDialogButtonBox::accepted, this,
          &AuthDialog::onButtonClicked);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

bool AuthDialog::eventFilter(QObject *obj, QEvent *event) {
  if (mode == "register" && obj == phoneEdit &&
      event->type() == QEvent::FocusIn) {
    if (phoneEdit->text().isEmpty()) {
      phoneEdit->setText("+7");
      phoneEdit->setCursorPosition(2);
    }
  }
  return QDialog::eventFilter(obj, event);
}

void AuthDialog::onButtonClicked() {
  if (mode == "login") {
    emit loginClicked(loginEdit->text(), passwordEdit->text());
  } else {
    emit registerClicked(loginEdit->text(), passwordEdit->text(),
                         emailEdit->text(), phoneEdit->text(),
                         privateCheckBox->isChecked());
  }
}

QString AuthDialog::getLogin() const { return loginEdit->text(); }
QString AuthDialog::getPassword() const { return passwordEdit->text(); }
QString AuthDialog::getEmail() const {
  return emailEdit ? emailEdit->text() : QString();
}
QString AuthDialog::getPhone() const {
  return phoneEdit ? phoneEdit->text() : QString();
}
bool AuthDialog::isPrivate() const {
  return privateCheckBox ? privateCheckBox->isChecked() : false;
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