/****************************************************************************
** Meta object code from reading C++ file 'feedwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../feedwindow.h"
#include <QtNetwork/QSslError>
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'feedwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_FeedWindow_t {
    uint offsetsAndSizes[46];
    char stringdata0[11];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[7];
    char stringdata4[14];
    char stringdata5[7];
    char stringdata6[7];
    char stringdata7[10];
    char stringdata8[7];
    char stringdata9[9];
    char stringdata10[13];
    char stringdata11[15];
    char stringdata12[21];
    char stringdata13[21];
    char stringdata14[20];
    char stringdata15[15];
    char stringdata16[6];
    char stringdata17[20];
    char stringdata18[19];
    char stringdata19[20];
    char stringdata20[22];
    char stringdata21[20];
    char stringdata22[22];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_FeedWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_FeedWindow_t qt_meta_stringdata_FeedWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "FeedWindow"
        QT_MOC_LITERAL(11, 15),  // "onAuthorClicked"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 6),  // "author"
        QT_MOC_LITERAL(35, 13),  // "onLikeDislike"
        QT_MOC_LITERAL(49, 6),  // "postId"
        QT_MOC_LITERAL(56, 6),  // "isLike"
        QT_MOC_LITERAL(63, 9),  // "loadPosts"
        QT_MOC_LITERAL(73, 6),  // "append"
        QT_MOC_LITERAL(80, 8),  // "loadMore"
        QT_MOC_LITERAL(89, 12),  // "onCreatePost"
        QT_MOC_LITERAL(102, 14),  // "onProfileClick"
        QT_MOC_LITERAL(117, 20),  // "onFindFriendsClicked"
        QT_MOC_LITERAL(138, 20),  // "onFriendsListClicked"
        QT_MOC_LITERAL(159, 19),  // "onPostReplyFinished"
        QT_MOC_LITERAL(179, 14),  // "QNetworkReply*"
        QT_MOC_LITERAL(194, 5),  // "reply"
        QT_MOC_LITERAL(200, 19),  // "onLoadPostsFinished"
        QT_MOC_LITERAL(220, 18),  // "onToggleFeedShared"
        QT_MOC_LITERAL(239, 19),  // "onToggleFeedFriends"
        QT_MOC_LITERAL(259, 21),  // "onProfileInfoFinished"
        QT_MOC_LITERAL(281, 19),  // "onFollowFromProfile"
        QT_MOC_LITERAL(301, 21)   // "onUnfollowFromProfile"
    },
    "FeedWindow",
    "onAuthorClicked",
    "",
    "author",
    "onLikeDislike",
    "postId",
    "isLike",
    "loadPosts",
    "append",
    "loadMore",
    "onCreatePost",
    "onProfileClick",
    "onFindFriendsClicked",
    "onFriendsListClicked",
    "onPostReplyFinished",
    "QNetworkReply*",
    "reply",
    "onLoadPostsFinished",
    "onToggleFeedShared",
    "onToggleFeedFriends",
    "onProfileInfoFinished",
    "onFollowFromProfile",
    "onUnfollowFromProfile"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_FeedWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  110,    2, 0x0a,    1 /* Public */,
       4,    2,  113,    2, 0x0a,    3 /* Public */,
       7,    1,  118,    2, 0x08,    6 /* Private */,
       7,    0,  121,    2, 0x28,    8 /* Private | MethodCloned */,
       9,    0,  122,    2, 0x08,    9 /* Private */,
      10,    0,  123,    2, 0x08,   10 /* Private */,
      11,    0,  124,    2, 0x08,   11 /* Private */,
      12,    0,  125,    2, 0x08,   12 /* Private */,
      13,    0,  126,    2, 0x08,   13 /* Private */,
      14,    1,  127,    2, 0x08,   14 /* Private */,
      17,    1,  130,    2, 0x08,   16 /* Private */,
      18,    0,  133,    2, 0x08,   18 /* Private */,
      19,    0,  134,    2, 0x08,   19 /* Private */,
      20,    1,  135,    2, 0x08,   20 /* Private */,
      21,    0,  138,    2, 0x08,   22 /* Private */,
      22,    0,  139,    2, 0x08,   23 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    5,    6,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject FeedWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_FeedWindow.offsetsAndSizes,
    qt_meta_data_FeedWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_FeedWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FeedWindow, std::true_type>,
        // method 'onAuthorClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLikeDislike'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'loadPosts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'loadPosts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadMore'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCreatePost'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProfileClick'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFindFriendsClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFriendsListClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPostReplyFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onLoadPostsFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onToggleFeedShared'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleFeedFriends'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProfileInfoFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onFollowFromProfile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onUnfollowFromProfile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void FeedWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FeedWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onAuthorClicked((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->onLikeDislike((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 2: _t->loadPosts((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->loadPosts(); break;
        case 4: _t->loadMore(); break;
        case 5: _t->onCreatePost(); break;
        case 6: _t->onProfileClick(); break;
        case 7: _t->onFindFriendsClicked(); break;
        case 8: _t->onFriendsListClicked(); break;
        case 9: _t->onPostReplyFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 10: _t->onLoadPostsFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 11: _t->onToggleFeedShared(); break;
        case 12: _t->onToggleFeedFriends(); break;
        case 13: _t->onProfileInfoFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 14: _t->onFollowFromProfile(); break;
        case 15: _t->onUnfollowFromProfile(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 10:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *FeedWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FeedWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FeedWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int FeedWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
