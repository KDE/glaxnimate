/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>

#include <QMetaType>
#include <QMetaMethod>
#include <QObject>

#include <QColor>
#include <QUuid>
#include <QVariant>
#include <QPointF>
#include <QSizeF>
#include <QVector2D>
#include <QRectF>
#include <QByteArray>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QGradient>

#include "glaxnimate/script/registrar_common.hpp"


namespace glaxnimate::script {


template<class T> const char* type_name()
{
    return QMetaType(qMetaTypeId<T>()).name();
}
template<int> struct meta_2_cpp_s;
template<class> struct cpp_2_meta_s;

#define TYPE_NAME(Type) //template<> const char* type_name<Type>() { return #Type; }
#define SETUP_TYPE(MetaInt, Type)                                   \
    TYPE_NAME(Type)                                                 \
    template<> struct meta_2_cpp_s<MetaInt> { using type = Type; }; \
    template<> struct cpp_2_meta_s<Type> { static constexpr const int value = MetaInt; };

template<int meta_type> using meta_2_cpp = typename meta_2_cpp_s<meta_type>::type;
template<class T> constexpr const int cpp_2_meta = cpp_2_meta_s<T>::value;

SETUP_TYPE(QMetaType::Int,          int)
SETUP_TYPE(QMetaType::Bool,         bool)
SETUP_TYPE(QMetaType::Double,       double)
SETUP_TYPE(QMetaType::Float,        float)
SETUP_TYPE(QMetaType::UInt,         unsigned int)
SETUP_TYPE(QMetaType::Long,         long)
SETUP_TYPE(QMetaType::LongLong,     long long)
SETUP_TYPE(QMetaType::Short,        short)
SETUP_TYPE(QMetaType::ULong,        unsigned long)
SETUP_TYPE(QMetaType::ULongLong,    unsigned long long)
SETUP_TYPE(QMetaType::UShort,       unsigned short)
SETUP_TYPE(QMetaType::QString,      QString)
SETUP_TYPE(QMetaType::QColor,       QColor)
SETUP_TYPE(QMetaType::QUuid,        QUuid)
SETUP_TYPE(QMetaType::QObjectStar,  QObject*)
SETUP_TYPE(QMetaType::QVariantList, QVariantList)
SETUP_TYPE(QMetaType::QVariant,     QVariant)
SETUP_TYPE(QMetaType::QStringList,  QStringList)
SETUP_TYPE(QMetaType::QVariantMap,  QVariantMap)
SETUP_TYPE(QMetaType::QVariantHash, QVariantHash)
SETUP_TYPE(QMetaType::QPointF,      QPointF)
SETUP_TYPE(QMetaType::QSizeF,       QSizeF)
SETUP_TYPE(QMetaType::QSize,        QSize)
SETUP_TYPE(QMetaType::QVector2D,    QVector2D)
SETUP_TYPE(QMetaType::QRectF,       QRectF)
SETUP_TYPE(QMetaType::QByteArray,   QByteArray)
SETUP_TYPE(QMetaType::QDateTime,    QDateTime)
SETUP_TYPE(QMetaType::QDate,        QDate)
SETUP_TYPE(QMetaType::QTime,        QTime)
SETUP_TYPE(QMetaType::QImage,       QImage)
// If you add stuff here, remember to add it to supported_types too

TYPE_NAME(std::vector<QObject*>)

using supported_types = std::integer_sequence<int,
    QMetaType::Bool,
    QMetaType::Int,
    QMetaType::Double,
    QMetaType::Float,
    QMetaType::UInt,
    QMetaType::Long,
    QMetaType::LongLong,
    QMetaType::Short,
    QMetaType::ULong,
    QMetaType::ULongLong,
    QMetaType::UShort,
    QMetaType::QString,
    QMetaType::QColor,
    QMetaType::QUuid,
    QMetaType::QObjectStar,
    QMetaType::QVariantList,
    QMetaType::QVariant,
    QMetaType::QStringList,
    QMetaType::QVariantMap,
    QMetaType::QVariantHash,
    QMetaType::QPointF,
    QMetaType::QSizeF,
    QMetaType::QSize,
    QMetaType::QVector2D,
    QMetaType::QRectF,
    QMetaType::QByteArray,
    QMetaType::QDateTime,
    QMetaType::QDate,
    QMetaType::QTime,
    QMetaType::QImage
    // Ensure new types have SETUP_TYPE
>;


template<int i, template<class FuncT> class Func, class RetT, class... FuncArgs>
bool type_dispatch_impl_step(int meta_type, RetT& ret, FuncArgs&&... args)
{
    if ( meta_type != i )
        return false;

    ret = Func<meta_2_cpp<i>>::do_the_thing(std::forward<FuncArgs>(args)...);
    return true;
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs, int... i>
bool type_dispatch_impl(int meta_type, RetT& ret, std::integer_sequence<int, i...>, FuncArgs&&... args)
{
    return (type_dispatch_impl_step<i, Func>(meta_type, ret, std::forward<FuncArgs>(args)...)||...);
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs>
RetT type_dispatch(int meta_type, FuncArgs&&... args)
{
    if ( meta_type >= QMetaType::User )
    {
        if ( QMetaType(meta_type).flags() & QMetaType::IsEnumeration )
            return Func<int>::do_the_thing(std::forward<FuncArgs>(args)...);
        return Func<QObject*>::do_the_thing(std::forward<FuncArgs>(args)...);
    }
    RetT ret;
    type_dispatch_impl<Func>(meta_type, ret, supported_types(), std::forward<FuncArgs>(args)...);
    return ret;
}

template<template<class FuncT> class Func, class RetT, class... FuncArgs>
static RetT type_dispatch_maybe_void(int meta_type, FuncArgs&&... args)
{
    if ( meta_type == QMetaType::Void )
        return Func<void>::do_the_thing(std::forward<FuncArgs>(args)...);
    return type_dispatch<Func, RetT>(meta_type, std::forward<FuncArgs>(args)...);
}


class ArgumentBuffer
{
public:
    ArgumentBuffer(const QMetaMethod& method) : method(method) {}
    ArgumentBuffer(const ArgumentBuffer&) = delete;
    ArgumentBuffer& operator=(const ArgumentBuffer&) = delete;
    ~ArgumentBuffer()
    {
        for ( int i = 0; i < destructors_used; i++)
        {
            destructors[i]->destruct();
            delete destructors[i];
        }
    }

    template<class CppType>
    const char* object_type_name(const CppType&)
    {
        return type_name<CppType>();
    }

    std::string object_type_name(QObject* value)
    {
        std::string s = value->metaObject()->className();
        std::string target = method.parameterTypes()[arguments].toStdString();
        if ( !target.empty() && target.back() == '*' )
        {
            target.pop_back();
            if ( s != target )
            {
                for ( auto mo = value->metaObject()->superClass(); mo; mo = mo->superClass() )
                {
                    std::string moname = mo->className();
                    if ( moname == target )
                        return target + "*";
                }
            }
        }
        return s + "*";
    }

    template<class CppType>
    CppType* allocate(const CppType& value)
    {
        if ( avail() < int(sizeof(CppType)) )
            throw ScriptError(i18n("Cannot allocate argument"));

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        names[arguments] = object_type_name(value);
        generic_args[arguments] = { names[arguments].c_str(), addr };
        ensure_destruction(addr);
        arguments += 1;
        *addr = value;
        return addr;
    }

    template<class CppType>
    void allocate_return_type(const char* name)
    {
        if ( avail() < int(sizeof(CppType)) )
            throw ScriptError(i18n("Cannot allocate return value"));

        CppType* addr = new (next_mem()) CppType;
        buffer_used += sizeof(CppType);
        ret = { name, addr };
        ensure_destruction(addr);
        ret_addr = addr;
    }

    template<class CppType>
    CppType return_value()
    {
        return *static_cast<CppType*>(ret_addr);
    }

    const QGenericArgument& arg(int i) const { return generic_args[i]; }

    const QGenericReturnArgument& return_arg() const { return ret; }

private:
    class Destructor
    {
    public:
        Destructor() = default;
        Destructor(const Destructor&) = delete;
        Destructor& operator=(const Destructor&) = delete;
        virtual ~Destructor() = default;
        virtual void destruct() const = 0;
    };

    template<class CppType>
    struct DestructorImpl : public Destructor
    {
        DestructorImpl(CppType* addr) : addr(addr) {}

        void destruct() const override
        {
            addr->~CppType();
        }

        CppType* addr;
    };

    int arguments = 0;
    int buffer_used = 0;
    std::array<char, 256> buffer;
    std::array<Destructor*, 16> destructors;
    std::array<QGenericArgument, 16> generic_args;
    std::array<std::string, 16> names;
    QGenericReturnArgument ret;
    void* ret_addr = nullptr;
    QMetaMethod method;
    int destructors_used = 0;


    int avail() { return buffer.size() - buffer_used; }
    void* next_mem() { return buffer.data() + buffer_used; }


    template<class CppType>
        std::enable_if_t<std::is_pod_v<CppType>> ensure_destruction(CppType*) {}


    template<class CppType>
        std::enable_if_t<!std::is_pod_v<CppType>> ensure_destruction(CppType* addr)
        {
            destructors[destructors_used] = new DestructorImpl<CppType>(addr);
            destructors_used++;
        }
};

template<> inline void ArgumentBuffer::allocate_return_type<void>(const char*){}
template<> inline void ArgumentBuffer::return_value<void>(){}

template<class CppType>
struct AllocateReturn
{
    static bool do_the_thing(script::ArgumentBuffer& buf, const char* name)
    {
        buf.allocate_return_type<CppType>(name);
        return true;
    }
};


template<class T> QVariant qvariant_from_cpp(const T& t) { return QVariant::fromValue(t); }
template<class T> T qvariant_to_cpp(const QVariant& v) { return v.value<T>(); }

template<> inline QVariant qvariant_from_cpp<std::string>(const std::string& t) { return QString::fromStdString(t); }
template<> inline std::string qvariant_to_cpp<std::string>(const QVariant& v) { return v.toString().toStdString(); }

template<> inline QVariant qvariant_from_cpp<QVariant>(const QVariant& t) { return t; }
template<> inline QVariant qvariant_to_cpp<QVariant>(const QVariant& v) { return v; }


template<> inline void qvariant_to_cpp<void>(const QVariant&) {}


template<>
QVariant inline qvariant_from_cpp<std::vector<QObject*>>(const std::vector<QObject*>& t)
{
    QVariantList list;
    for ( QObject* obj : t )
        list.push_back(QVariant::fromValue(obj));
    return list;

}
template<> inline std::vector<QObject*> qvariant_to_cpp<std::vector<QObject*>>(const QVariant& v)
{
    std::vector<QObject*> objects;
    for ( const QVariant& vi : v.toList() )
        objects.push_back(vi.value<QObject*>());
    return objects;
}

} // namespace glaxnimate::script
