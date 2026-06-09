#include "../postdialog.h"
#include <QLineEdit>
#include <QTextEdit>
#include <QtTest/QtTest>

class TestPostDialog : public QObject {
    Q_OBJECT
private slots:
    void testInitialState() {
        PostDialog dialog;
        QCOMPARE(dialog.getDescription(), QString(""));
        QCOMPARE(dialog.getTags().size(), 0);
    }

    void testTagParsing() {
        PostDialog dialog;
        QLineEdit *tagsEdit = dialog.findChild<QLineEdit *>();
        QVERIFY(tagsEdit != nullptr);

        tagsEdit->setText(" #cpp #qt6 #tests ");
        QStringList tags = dialog.getTags();

        QCOMPARE(tags.size(), 3);
        QCOMPARE(tags.at(0), QString("#cpp"));
        QCOMPARE(tags.at(2), QString("#tests"));
    }
};

int runPostDialogTests(int argc, char *argv[]) {
    TestPostDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_postdialog.moc"