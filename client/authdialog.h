#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(const QString &mode, QWidget *parent = nullptr);
    QString getLogin() const;
    QString getPassword() const;
    QString getEmail() const;
    QString getPhone() const;
    bool isPrivate() const;
    void clearField(const QString &fieldName);

signals:
    void loginClicked(const QString &login, const QString &password);
    void registerClicked(const QString &login, const QString &password, const QString &email,
                         const QString &phone, bool isPrivate);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onButtonClicked();

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QCheckBox *privateCheckBox;
    QString mode;
    void setupUI();
};

#endif