#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(const QString &mode, QWidget *parent = nullptr);
    QString getNickname() const;
    QString getLogin() const;
    QString getPassword() const;
    QString getEmail() const;
    QString getPhone() const;
    bool isPublic() const;

private:
    QLineEdit *nicknameEdit;
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QCheckBox *isPublicCheckBox;
};

#endif