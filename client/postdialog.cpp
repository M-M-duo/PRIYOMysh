#include "postdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QImage>
#include <QBuffer>
#include <QRegularExpression>

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

    chooseImageButton = new QPushButton("Choose Image", this);
    connect(chooseImageButton, &QPushButton::clicked, this, &PostDialog::chooseImage);
    layout->addWidget(chooseImageButton);

    imageLabel = new QLabel("No image selected", this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(80);
    imageLabel->setStyleSheet("border: 1px solid gray;");
    layout->addWidget(imageLabel);

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

void PostDialog::chooseImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;

    imageBase64 = cropAndToBase64(filePath);
    if (imageBase64.isEmpty()) {
        imageLabel->setText("Failed to load image");
        return;
    }
    imageLabel->setText("Image loaded");
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

QString PostDialog::getDescription() const {
    return descriptionEdit->toPlainText();
}

QString PostDialog::getImageBase64() const {
    return imageBase64;
}

QStringList PostDialog::getTags() const {
    QString raw = tagsEdit->text().trimmed();
    if (raw.isEmpty()) return QStringList();
    return raw.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}