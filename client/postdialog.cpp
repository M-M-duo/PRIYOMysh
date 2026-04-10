#include "postdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QImage>
#include <QBuffer>
#include <QRegularExpression>
#include <QMessageBox>
#include <QPixmap>
#include <QEvent>

PostDialog::PostDialog(QWidget *parent) : QDialog(parent) {
    setupUI();
}

void PostDialog::setupUI() {
    setWindowTitle("Create Post");
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Description:"));
    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setMaximumHeight(100);
    layout->addWidget(descriptionEdit);

    layout->addWidget(new QLabel("Images (max 10):"));
    QHBoxLayout *imagesLayout = new QHBoxLayout();
    for (int i = 0; i < 10; ++i) {
        QLabel *slot = new QLabel(this);
        slot->setFixedSize(60, 60);
        slot->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
        slot->setAlignment(Qt::AlignCenter);
        slot->setScaledContents(true);
        slot->installEventFilter(this);
        imagesLayout->addWidget(slot);
        imageSlots.append(slot);
    }
    layout->addLayout(imagesLayout);

    layout->addWidget(new QLabel("Tags (space separated):"));
    tagsEdit = new QLineEdit(this);
    layout->addWidget(tagsEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    publishButton = new QPushButton("Publish", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(publishButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(publishButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

bool PostDialog::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        int index = imageSlots.indexOf(static_cast<QLabel*>(obj));
        if (index != -1) {
            chooseImage(index);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void PostDialog::chooseImage(int index) {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;

    QString base64 = cropAndToBase64(filePath);
    if (base64.isEmpty()) {
        QMessageBox::warning(this, "Error", "Failed to load image");
        return;
    }
    if (index < imagesBase64.size()) {
        imagesBase64[index] = base64;
    } else {
        imagesBase64.append(base64);
    }
    updateImageSlot(index, base64);
}

QString PostDialog::cropAndToBase64(const QString &filePath) {
    QImage image(filePath);
    if (image.isNull()) return QString();

    int size = qMin(image.width(), image.height());
    int x = (image.width() - size) / 2;
    int y = (image.height() - size) / 2;
    QImage cropped = image.copy(x, y, size, size);

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    cropped.save(&buffer, "PNG");
    buffer.close();

    return QString::fromLatin1(byteArray.toBase64());
}

void PostDialog::updateImageSlot(int index, const QString &base64) {
    QLabel *slot = imageSlots[index];
    if (!base64.isEmpty()) {
        QPixmap pixmap;
        pixmap.loadFromData(QByteArray::fromBase64(base64.toLatin1()));
        slot->setPixmap(pixmap.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        slot->setStyleSheet("border: 1px solid pink; background-color: pink;");
    } else {
        slot->clear();
        slot->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
    }
}

QString PostDialog::getDescription() const {
    return descriptionEdit->toPlainText();
}

QStringList PostDialog::getImagesBase64() const {
    return imagesBase64;
}

QStringList PostDialog::getTags() const {
    QString raw = tagsEdit->text().trimmed();
    if (raw.isEmpty()) return QStringList();
    return raw.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}