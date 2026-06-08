#include <QApplication>
#include <QtTest/QtTest>

int runMainWindowTests(int argc, char *argv[]);
int runAuthDialogTests(int argc, char *argv[]);
int runPostDialogTests(int argc, char *argv[]);
int runEditProfileDialogTests(int argc, char *argv[]);
int runFeedWindowTests(int argc, char *argv[]);
int runFriendFinderTests(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    int status = 0;

    status |= runMainWindowTests(argc, argv);
    status |= runAuthDialogTests(argc, argv);
    status |= runPostDialogTests(argc, argv);
    status |= runEditProfileDialogTests(argc, argv);
    status |= runFeedWindowTests(argc, argv);
    status |= runFriendFinderTests(argc, argv);

    return status;
}