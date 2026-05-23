#ifndef EDITPROFILEDIALOG_H
#define EDITPROFILEDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QLineEdit>

class EditProfileDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditProfileDialog(const QString &login, const QString &email, const QString &phone,
                               bool isPrivate, QWidget *parent = nullptr);
    QString getLogin() const;
    QString getEmail() const;
    QString getPhone() const;
    bool isPrivate() const;

private slots:
    void onSaveClicked();

signals:
    void profileUpdated(const QString &login, const QString &email, const QString &phone,
                        bool isPrivate);

private:
    QLineEdit *loginEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QCheckBox *privateCheckBox;
    void setupUI();
};

#endif