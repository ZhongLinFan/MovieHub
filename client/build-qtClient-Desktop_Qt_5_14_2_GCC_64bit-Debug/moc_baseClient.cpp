/****************************************************************************
** Meta object code from reading C++ file 'baseclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../qtClient/baseclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'baseclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BaseClient_t {
    QByteArrayData data[13];
    char stringdata0[161];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BaseClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BaseClient_t qt_meta_stringdata_BaseClient = {
    {
QT_MOC_LITERAL(0, 0, 10), // "BaseClient"
QT_MOC_LITERAL(1, 11, 16), // "switchMainWindow"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 11), // "MainWindow*"
QT_MOC_LITERAL(4, 41, 10), // "mainWindow"
QT_MOC_LITERAL(5, 52, 18), // "updateFavoriteList"
QT_MOC_LITERAL(6, 71, 16), // "updateFriendList"
QT_MOC_LITERAL(7, 88, 15), // "updateGroupList"
QT_MOC_LITERAL(8, 104, 19), // "sendMsgResultNotify"
QT_MOC_LITERAL(9, 124, 6), // "fromId"
QT_MOC_LITERAL(10, 131, 6), // "result"
QT_MOC_LITERAL(11, 138, 16), // "recevidMsgNotify"
QT_MOC_LITERAL(12, 155, 5) // "modid"

    },
    "BaseClient\0switchMainWindow\0\0MainWindow*\0"
    "mainWindow\0updateFavoriteList\0"
    "updateFriendList\0updateGroupList\0"
    "sendMsgResultNotify\0fromId\0result\0"
    "recevidMsgNotify\0modid"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BaseClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       5,    0,   47,    2, 0x06 /* Public */,
       6,    0,   48,    2, 0x06 /* Public */,
       7,    0,   49,    2, 0x06 /* Public */,
       8,    2,   50,    2, 0x06 /* Public */,
      11,    2,   55,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    9,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   12,    9,

       0        // eod
};

void BaseClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BaseClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->switchMainWindow((*reinterpret_cast< MainWindow*(*)>(_a[1]))); break;
        case 1: _t->updateFavoriteList(); break;
        case 2: _t->updateFriendList(); break;
        case 3: _t->updateGroupList(); break;
        case 4: _t->sendMsgResultNotify((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->recevidMsgNotify((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BaseClient::*)(MainWindow * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::switchMainWindow)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BaseClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::updateFavoriteList)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BaseClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::updateFriendList)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BaseClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::updateGroupList)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (BaseClient::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::sendMsgResultNotify)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (BaseClient::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BaseClient::recevidMsgNotify)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BaseClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_BaseClient.data,
    qt_meta_data_BaseClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BaseClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BaseClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BaseClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BaseClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void BaseClient::switchMainWindow(MainWindow * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BaseClient::updateFavoriteList()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void BaseClient::updateFriendList()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void BaseClient::updateGroupList()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void BaseClient::sendMsgResultNotify(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void BaseClient::recevidMsgNotify(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
