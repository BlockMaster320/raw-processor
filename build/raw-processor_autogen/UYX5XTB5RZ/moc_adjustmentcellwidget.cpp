/****************************************************************************
** Meta object code from reading C++ file 'adjustmentcellwidget.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ui/adjustmentcellwidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'adjustmentcellwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN20AdjustmentCellWidgetE_t {};
} // unnamed namespace

template <> constexpr inline auto AdjustmentCellWidget::qt_create_metaobjectdata<qt_meta_tag_ZN20AdjustmentCellWidgetE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AdjustmentCellWidget",
        "adjustmentChanged",
        "",
        "sliderReleased",
        "AdjustmentCell*",
        "cell",
        "cellActivated",
        "dragInitiated",
        "AdjustmentCellWidget*",
        "widget",
        "globalPos",
        "cellRenameRequested",
        "newName",
        "removeCellRequested",
        "unlinkCellRequested"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'adjustmentChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'sliderReleased'
        QtMocHelpers::SignalData<void(AdjustmentCell *)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'cellActivated'
        QtMocHelpers::SignalData<void(AdjustmentCell *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'dragInitiated'
        QtMocHelpers::SignalData<void(AdjustmentCellWidget *, QPoint)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 }, { QMetaType::QPoint, 10 },
        }}),
        // Signal 'cellRenameRequested'
        QtMocHelpers::SignalData<void(AdjustmentCell *, const QString &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 }, { QMetaType::QString, 12 },
        }}),
        // Signal 'removeCellRequested'
        QtMocHelpers::SignalData<void(AdjustmentCell *)>(13, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'unlinkCellRequested'
        QtMocHelpers::SignalData<void(AdjustmentCell *)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AdjustmentCellWidget, qt_meta_tag_ZN20AdjustmentCellWidgetE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AdjustmentCellWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20AdjustmentCellWidgetE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20AdjustmentCellWidgetE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN20AdjustmentCellWidgetE_t>.metaTypes,
    nullptr
} };

void AdjustmentCellWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AdjustmentCellWidget *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->adjustmentChanged(); break;
        case 1: _t->sliderReleased((*reinterpret_cast< std::add_pointer_t<AdjustmentCell*>>(_a[1]))); break;
        case 2: _t->cellActivated((*reinterpret_cast< std::add_pointer_t<AdjustmentCell*>>(_a[1]))); break;
        case 3: _t->dragInitiated((*reinterpret_cast< std::add_pointer_t<AdjustmentCellWidget*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 4: _t->cellRenameRequested((*reinterpret_cast< std::add_pointer_t<AdjustmentCell*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->removeCellRequested((*reinterpret_cast< std::add_pointer_t<AdjustmentCell*>>(_a[1]))); break;
        case 6: _t->unlinkCellRequested((*reinterpret_cast< std::add_pointer_t<AdjustmentCell*>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< AdjustmentCellWidget* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)()>(_a, &AdjustmentCellWidget::adjustmentChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCell * )>(_a, &AdjustmentCellWidget::sliderReleased, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCell * )>(_a, &AdjustmentCellWidget::cellActivated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCellWidget * , QPoint )>(_a, &AdjustmentCellWidget::dragInitiated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCell * , const QString & )>(_a, &AdjustmentCellWidget::cellRenameRequested, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCell * )>(_a, &AdjustmentCellWidget::removeCellRequested, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AdjustmentCellWidget::*)(AdjustmentCell * )>(_a, &AdjustmentCellWidget::unlinkCellRequested, 6))
            return;
    }
}

const QMetaObject *AdjustmentCellWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdjustmentCellWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN20AdjustmentCellWidgetE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int AdjustmentCellWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void AdjustmentCellWidget::adjustmentChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AdjustmentCellWidget::sliderReleased(AdjustmentCell * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void AdjustmentCellWidget::cellActivated(AdjustmentCell * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void AdjustmentCellWidget::dragInitiated(AdjustmentCellWidget * _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void AdjustmentCellWidget::cellRenameRequested(AdjustmentCell * _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void AdjustmentCellWidget::removeCellRequested(AdjustmentCell * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void AdjustmentCellWidget::unlinkCellRequested(AdjustmentCell * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}
QT_WARNING_POP
