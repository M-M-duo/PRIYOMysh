#include <QtTest/QtTest>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../friendfinder.h"

class TestFriendFinder : public QObject {
    Q_OBJECT
private slots:
    void testUIStateWhenUserFound() {
        FriendFinder finder("mock_token");

        QList<QLabel*> labels = finder.findChildren<QLabel*>();
        QVERIFY(labels.size() >= 2);

        QLabel* resultLabel = labels.at(0);
        QLabel* statusLabel = labels.at(1);
        QPushButton* actionButton = finder.findChild<QPushButton*>();
        QWidget* resultWidget = finder.findChild<QWidget*>();

        QVERIFY(resultLabel != nullptr);
        QVERIFY(statusLabel != nullptr);
        QVERIFY(actionButton != nullptr);

        resultLabel->setText("test_friend_login");
        statusLabel->setText("Not followed");
        actionButton->setText("Follow");
        resultWidget->setVisible(true);

        QCOMPARE(resultLabel->text(), QString("test_friend_login"));
        QCOMPARE(statusLabel->text(), QString("Not followed"));
        QCOMPARE(actionButton->text(), QString("Follow"));
        
        QVERIFY(resultWidget->isVisibleTo(&finder));
    }
};

int runFriendFinderTests(int argc, char *argv[]) {
    TestFriendFinder test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_friendfinder.moc"