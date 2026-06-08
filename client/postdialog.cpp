#include "postdialog.h"
#include <QBuffer>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QRegularExpression>
#include <QVBoxLayout>

const int MAX_DESCRIPTION_LENGTH = 1000;

PostDialog::PostDialog(QWidget *parent) : QDialog(parent) {
    setupUI();
}

void PostDialog::setupUI() {
    setWindowTitle("Create Post");
    setFixedWidth(350);

    move(50, 180);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Description:"));
    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    descriptionEdit->setMinimumHeight(80);
    connect(descriptionEdit, &QTextEdit::textChanged, this, &PostDialog::onDescriptionChanged);
    layout->addWidget(descriptionEdit);

    layout->addWidget(new QLabel("Images (max 10):"));

    imagesLayout = new QGridLayout();
    for (int i = 0; i < 10; ++i) {
        QLabel *slot = new QLabel(this);
        slot->setFixedSize(60, 60);
        slot->setStyleSheet("border: 1px solid gray; background-color: #f0f0f0;");
        slot->setAlignment(Qt::AlignCenter);
        slot->setScaledContents(true);
        slot->installEventFilter(this);
        imagesLayout->addWidget(slot, i / 5, i % 5);
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

void PostDialog::onDescriptionChanged() {
    QString currentText = descriptionEdit->toPlainText();
    if (currentText.length() > MAX_DESCRIPTION_LENGTH) {
        descriptionEdit->blockSignals(true);
        descriptionEdit->setPlainText(lastValidDescription);
        descriptionEdit->blockSignals(false);
    } else {
        lastValidDescription = currentText;
    }
}

bool PostDialog::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        int index = imageSlots.indexOf(static_cast<QLabel *>(obj));
        if (index != -1) {
            chooseImage(index);
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

bool PostDialog::isValidImage(const QString &filePath) {
    QString ext = QFileInfo(filePath).suffix().toLower();
    return (ext == "jpg" || ext == "jpeg" || ext == "png");
}

void PostDialog::chooseImage(int index) {
    QStringList filePaths =
        QFileDialog::getOpenFileNames(this, "Select Images", "", "Images (*.png *.jpg *.jpeg)");

    if (filePaths.isEmpty())
        return;

    int currentSlot = index;
    for (const QString &path : filePaths) {
        if (currentSlot >= 10)
            break;

        if (!isValidImage(path)) {
            QMessageBox::warning(this, "Error",
                                 "Unsupported format: " + QFileInfo(path).fileName());
            continue;
        }

        QString base64 = cropAndToBase64(path);
        if (base64.isEmpty())
            continue;

        if (currentSlot < imagesBase64.size()) {
            imagesBase64[currentSlot] = base64;
        } else {
            imagesBase64.append(base64);
        }

        updateImageSlot(currentSlot, base64);
        currentSlot++;
    }
}

QString PostDialog::cropAndToBase64(const QString &filePath) {
    QImage image(filePath);
    if (image.isNull())
        return QString();

    int size = qMin(image.width(), image.height());
    int x = (image.width() - size) / 2;
    int y = (image.height() - size) / 2;
    QImage cropped = image.copy(x, y, size, size);

    const int maxSize = 500;
    if (cropped.width() > maxSize || cropped.height() > maxSize) {
        cropped = cropped.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    cropped.save(&buffer, "JPEG", 85);
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
    if (raw.isEmpty())
        return QStringList();
    return raw.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}