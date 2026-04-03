#ifndef POSTDIALOG_H
#define POSTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class PostDialog : public QDialog {
    Q_OBJECT
public:
    explicit PostDialog(QWidget *parent = nullptr);
    QString getDescription() const;
    QString getImageBase64() const;
    QStringList getTags() const;

private slots:
    void chooseImage();

private:
    QTextEdit *descriptionEdit;
    QLineEdit *tagsEdit;
    QLabel *imageLabel;
    QString imageBase64;
    QPushButton *chooseImageButton;
    QPushButton *publishButton;
    QPushButton *cancelButton;

    void setupUI();
    QString cropAndToBase64(const QString &filePath);
};

#endif // POSTDIALOG_H