#ifndef EDITPROFILEDIALOG_H
#define EDITPROFILEDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class EditProfileDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditProfileDialog(const QString &login, const QString &email, const QString &phone,
                               bool isPrivate, const QString &avatarBase64,
                               const QString &authToken, QWidget *parent = nullptr);
    ~EditProfileDialog();

signals:
    void profileUpdated(const QString &login, const QString &email, const QString &phone,
                        bool isPrivate, const QString &avatarBase64);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onSaveClicked();
    void onChangePasswordClicked();
    void onPasswordUpdated();

private:
    QLineEdit *loginEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QCheckBox *privateCheckBox;
    QLabel *avatarLabel;
    QString avatarBase64;
    QString authToken;
    QPushButton *changePasswordButton;
    QLineEdit *currentPasswordEdit;
    QLineEdit *newPasswordEdit;
    QPushButton *updatePasswordButton;
    QPushButton *cancelPasswordButton;
    QWidget *passwordWidget;

    void setupUI();
    void setupPasswordUI();
    QString cropAndToBase64(const QString &filePath);
    void chooseAvatar();
    void showMessage(const QString &text, const QString &iconPath);
};

#endif // EDITPROFILEDIALOG_H