/****************************************************************************
** Meta object code from reading C++ file 'friendfinder.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../friendfinder.h"
#include <QtNetwork/QSslError>
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'friendfinder.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_FriendFinder_t {
    uint offsetsAndSizes[22];
    char stringdata0[13];
    char stringdata1[11];
    char stringdata2[1];
    char stringdata3[11];
    char stringdata4[13];
    char stringdata5[17];
    char stringdata6[15];
    char stringdata7[6];
    char stringdata8[17];
    char stringdata9[19];
    char stringdata10[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_FriendFinder_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_FriendFinder_t qt_meta_stringdata_FriendFinder = {
    {
        QT_MOC_LITERAL(0, 12),  // "FriendFinder"
        QT_MOC_LITERAL(13, 10),  // "searchUser"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 10),  // "followUser"
        QT_MOC_LITERAL(36, 12),  // "unfollowUser"
        QT_MOC_LITERAL(49, 16),  // "onSearchFinished"
        QT_MOC_LITERAL(66, 14),  // "QNetworkReply*"
        QT_MOC_LITERAL(81, 5),  // "reply"
        QT_MOC_LITERAL(87, 16),  // "onFollowFinished"
        QT_MOC_LITERAL(104, 18),  // "onUnfollowFinished"
        QT_MOC_LITERAL(123, 11)   // "onViewPosts"
    },
    "FriendFinder",
    "searchUser",
    "",
    "followUser",
    "unfollowUser",
    "onSearchFinished",
    "QNetworkReply*",
    "reply",
    "onFollowFinished",
    "onUnfollowFinished",
    "onViewPosts"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_FriendFinder[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x08,    1 /* Private */,
       3,    0,   57,    2, 0x08,    2 /* Private */,
       4,    0,   58,    2, 0x08,    3 /* Private */,
       5,    1,   59,    2, 0x08,    4 /* Private */,
       8,    1,   62,    2, 0x08,    6 /* Private */,
       9,    1,   65,    2, 0x08,    8 /* Private */,
      10,    0,   68,    2, 0x08,   10 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject FriendFinder::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_FriendFinder.offsetsAndSizes,
    qt_meta_data_FriendFinder,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_FriendFinder_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FriendFinder, std::true_type>,
        // method 'searchUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'followUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'unfollowUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSearchFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onFollowFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onUnfollowFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>,
        // method 'onViewPosts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void FriendFinder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FriendFinder *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->searchUser(); break;
        case 1: _t->followUser(); break;
        case 2: _t->unfollowUser(); break;
        case 3: _t->onSearchFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 4: _t->onFollowFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 5: _t->onUnfollowFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 6: _t->onViewPosts(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
}

const QMetaObject *FriendFinder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FriendFinder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FriendFinder.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int FriendFinder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
