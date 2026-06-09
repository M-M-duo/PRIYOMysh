#include "../editprofiledialog.h"
#include <QCheckBox>
#include <QLineEdit>
#include <QtTest/QtTest>

class TestEditProfileDialog : public QObject {
    Q_OBJECT
private slots:
    void testProfileDataLoading() {
        EditProfileDialog dialog("maruuskin", "test@hse.ru", "+79991112233", true, "", "token123");

        QList<QLineEdit *> edits = dialog.findChildren<QLineEdit *>();

        bool foundLogin = false;
        bool foundEmail = false;
        for (auto *edit : edits) {
            if (edit->text() == "maruuskin")
                foundLogin = true;
            if (edit->text() == "test@hse.ru")
                foundEmail = true;
        }

        QVERIFY(foundLogin);
        QVERIFY(foundEmail);

        QCheckBox *privateCheck = dialog.findChild<QCheckBox *>();
        QVERIFY(privateCheck != nullptr);
        QVERIFY(privateCheck->isChecked());
    }
};

int runEditProfileDialogTests(int argc, char *argv[]) {
    TestEditProfileDialog test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_editprofiledialog.moc"