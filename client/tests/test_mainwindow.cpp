#include "../authdialog.h"
#include "../mainwindow.h"
#include <QPushButton>
#include <QtTest/QtTest>

class TestMainWindow : public QObject {
    Q_OBJECT
private slots:
    void testButtonsExist() {
        MainWindow mw;
        QList<QPushButton *> buttons = mw.findChildren<QPushButton *>();
        QVERIFY(buttons.size() >= 2);

        bool hasLogin = false;
        bool hasRegister = false;
        for (auto *btn : buttons) {
            if (btn->text() == "Login")
                hasLogin = true;
            if (btn->text() == "Register")
                hasRegister = true;
        }
        QVERIFY(hasLogin);
        QVERIFY(hasRegister);
    }

    void testDialogSpawning() {
        MainWindow mw;
        mw.show();

        QPushButton *loginBtn = nullptr;
        for (auto *btn : mw.findChildren<QPushButton *>()) {
            if (btn->text() == "Login") {
                loginBtn = btn;
                break;
            }
        }
        QVERIFY(loginBtn != nullptr);

        QTest::mouseClick(loginBtn, Qt::LeftButton);

        QDialog *dialog = mw.findChild<AuthDialog *>();
        QVERIFY(dialog != nullptr);
        dialog->close();
    }
};

int runMainWindowTests(int argc, char *argv[]) {
    TestMainWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_mainwindow.moc"