#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include <QJsonObject>

class ProfileDialog : public QDialog {
    Q_OBJECT
public:
    explicit ProfileDialog(const QJsonObject &userData, QWidget *parent = nullptr);

private:
    void setupUI(const QJsonObject &userData);
};

#endif