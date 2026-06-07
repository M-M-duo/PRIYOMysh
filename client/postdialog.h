#ifndef POSTDIALOG_H
#define POSTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVector>

class PostDialog : public QDialog {
    Q_OBJECT
public:
    explicit PostDialog(QWidget *parent = nullptr);
    QString getDescription() const;
    QStringList getImagesBase64() const;
    QStringList getTags() const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void chooseImage(int index);

private:
    QTextEdit *descriptionEdit;
    QLineEdit *tagsEdit;
    QVector<QLabel*> imageSlots;
    QStringList imagesBase64;
    QPushButton *publishButton;
    QPushButton *cancelButton;

    void setupUI();
    QString cropAndToBase64(const QString &filePath);
    void updateImageSlot(int index, const QString &base64);
};

#endif