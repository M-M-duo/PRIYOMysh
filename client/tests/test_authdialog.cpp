#include <QtTest/QtTest>
#include <QLineEdit>
#include <QPushButton>
#include "../authdialog.h"

class TestAuthDialog : public QObject {
    Q_OBJECT
private slots:
    void testLoginModeInitialization() {
        AuthDialog dialog("login");
        QList<QLineEdit*> fields = dialog.findChildren<QLineEdit*>();
        QCOMPARE(fields.size(), 2); 
    }

    void testRegisterSignals() {
        AuthDialog dialog("register");
        QSignalSpy spy(&dialog, &AuthDialog::registerClicked);

        QLineEdit* loginEdit = dialog.findChild<QLineEdit*>();
        QPushButton* submitBtn = dialog.findChild<QPushButton*>();

        if (loginEdit && submitBtn) {
            loginEdit->setText("testuser");
            submitBtn->click();
            QCOMPARE(spy.count(), 1);
        }
    }
};

int runAuthDialogTests(int argc, char *argv[]) {
    TestAuthDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_authdialog.moc"