#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include <QLineEdit>

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(const QString &mode, QWidget *parent = nullptr);
    QString getNickname() const;
    QString getLogin() const;
    QString getPassword() const;

private:
    QLineEdit *nicknameEdit;
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
};

#endif // AUTHDIALOG_H