#include "../feedwindow.h"
#include <QScrollArea>
#include <QtTest/QtTest>

class TestFeedWindow : public QObject {
    Q_OBJECT

private slots:
    void testFeedLayoutStructure() {
        FeedWindow feedWin("mock_token_xyz");

        feedWin.resize(400, 700);

        QScrollArea *scrollArea = feedWin.findChild<QScrollArea *>();
        QVERIFY2(scrollArea != nullptr, "Scroll Area for layout feed posts was not found.");

        QVERIFY(feedWin.width() > 300);
        QVERIFY(feedWin.height() > 600);
    }
};

int runFeedWindowTests(int argc, char *argv[]) {
    TestFeedWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_feedwindow.moc"