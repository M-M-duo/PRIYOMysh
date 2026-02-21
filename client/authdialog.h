#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class AuthDialog : public QDialog {
    Q_OBJECT

public:
    explicit AuthDialog(const QString &mode, QWidget *parent = nullptr);
    QString getLogin() const;
    QString getPassword() const;

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QPushButton *actionButton;
};

#endif // AUTHDIALOG_H