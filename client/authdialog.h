#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>

class AuthDialog : public QDialog {
    Q_OBJECT
public:
    explicit AuthDialog(const QString &mode, QWidget *parent = nullptr);
    
    void enableButtons(bool enabled);

    QString getLogin() const;
    QString getPassword() const;
    QString getEmail() const;
    QString getPhone() const;
    bool isPrivate() const;
    QString getAvatarBase64() const;
    void clearField(const QString &fieldName);

signals:
    void loginClicked(const QString &login, const QString &password);
    void registerClicked(const QString &login, const QString &password, const QString &email,
                         const QString &phone, bool isPrivate, const QString &avatarBase64);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onButtonClicked();
    void chooseAvatar();

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QCheckBox *privateCheckBox;
    QLabel *avatarLabel;
    QString avatarBase64;
    QString mode;

    void setupUI();
    QString cropAndToBase64(const QString &filePath);
};

#endif // AUTHDIALOG_H