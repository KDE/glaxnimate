/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once
#include <functional>
#include <emscripten/bind.h>
#include <QMetaProperty>
#include <QColor>
#include "glaxnimate/model/document.hpp"
#include "glaxnimate/script/register_machinery.hpp"


namespace emscripten {


// newer version of embind have policies in val::vall but we are restricted by Qt
template<class T, class... Policies>
emscripten::val val_ptr(T* ptr, Policies...)
{
    return emscripten::val(ptr); // Why the heck does this work?
    // return emscripten::val::take_ownership(internal::_emval_take_value(internal::TypeID<QObject>::get(), ptr));
}

} // namespace emscripten

namespace glaxnimate::js {


template<class EnumT>
/*PyEnumInfo*/void register_enum(const QMetaEnum& meta)
{
    emscripten::enum_<EnumT> pyenum(meta.name());
    for ( int i = 0; i < meta.keyCount(); i++ )
        pyenum.value(meta.key(i), EnumT(meta.value(i)));

    // return {meta.name(), pyenum};
}

template<class... Enums>
struct enums;

template<class EnumT, class... Others>
struct enums<EnumT, Others...>
    : public enums<Others...>
{
    void process(/*std::vector<PyEnumInfo>& out*/)
    {
        /*out.push_back(*/register_enum<EnumT>(QMetaEnum::fromType<EnumT>())/*)*/;
        enums<Others...>::process(/*out*/);
    }
};

template<>
struct enums<>
{
    void process(/*std::vector<PyEnumInfo>&*/) {}
};

template<class T>
struct FunctionInfo : public FunctionInfo<decltype(&T::operator())> {};

template <typename Class, typename Return, typename... Args>
struct FunctionInfo<Return(Class::*)(Args...) const>
{
    using return_type = Return;
    static constexpr const int num_args = sizeof...(Args);
    using wrapper_type = std::function<Return(Args...)>;
};

template<class T>
decltype(auto) fn(const T& func)
{
    return typename FunctionInfo<T>::wrapper_type(func);
}

emscripten::val bytearray_to_val(const QByteArray& cpp_arr, bool alias)
{
    // Get a refence to the byte array buffer
    emscripten::val unalaiased = emscripten::val(
        emscripten::typed_memory_view(
            cpp_arr.size(),
            reinterpret_cast<const uint8_t*>(cpp_arr.constData())
        )
    );
    if ( !alias )
        return unalaiased;
    // Make sure the data is actually copied, rather than referenced
    auto js_arr = emscripten::val::global("Uint8Array").new_(cpp_arr.size());
    js_arr.call<void>("set", unalaiased);
    return js_arr;
}

QByteArray bytearray_from_val(const emscripten::val& val)
{
    if ( val.instanceof(emscripten::val::global("ArrayBuffer")) )
    {
        return bytearray_from_val(emscripten::val::global("Uint8Array").new_(val));
    }
    std::vector<char> vec = emscripten::convertJSArrayToNumberVector<char>(val);
    return QByteArray(vec.data(), vec.size());
}

bool is_byte_array(const emscripten::val& val)
{
    return val.instanceof(emscripten::val::global("Uint8Array"))
        || val.instanceof(emscripten::val::global("Int8Array"))
        || val.instanceof(emscripten::val::global("ArrayBuffer"))
        ;
}

emscripten::val qvariant_to_val(const QVariant& v)
{
    switch ( v.typeId() )
    {
        case QMetaType::Bool:
            return emscripten::val(v.toBool());
        case QMetaType::Float:
            return emscripten::val(v.toFloat());
        case QMetaType::Double:
            return emscripten::val(v.toDouble());
        case QMetaType::Long:
        case QMetaType::Short:
        case QMetaType::Int:
            return emscripten::val(v.toInt());
        case QMetaType::LongLong:
            return emscripten::val(v.toLongLong());
        case QMetaType::ULong:
        case QMetaType::UShort:
        case QMetaType::UInt:
            return emscripten::val(v.toUInt());
        case QMetaType::ULongLong:
            return emscripten::val(v.toULongLong());
        case QMetaType::QString:
        case QMetaType::QUuid:
            return emscripten::val(v.toString());
        case QMetaType::QVariantList:
        {
            auto js_arr = emscripten::val::array();
            auto cpp_arr = v.toList();
            for ( int i = 0; i < cpp_arr.size(); i++ )
                js_arr.set(i, qvariant_to_val(cpp_arr[i]));
            return js_arr;
        }
        case QMetaType::QColor:
            return emscripten::val(qvariant_cast<QColor>(v));
        case QMetaType::QPointF:
            return emscripten::val(qvariant_cast<QPointF>(v));
        case QMetaType::QSizeF:
            return emscripten::val(qvariant_cast<QSizeF>(v));
        case QMetaType::QVector2D:
            return emscripten::val(qvariant_cast<QVector2D>(v));
        case QMetaType::QByteArray:
            return bytearray_to_val(v.toByteArray(), true);
    }

    if ( v.metaType().flags() & QMetaType::IsEnumeration )
    {
        return emscripten::val(v.toInt());
    }

    auto meta_obj = v.metaType().metaObject();
    if ( meta_obj )
    {
        return emscripten::val_ptr(qvariant_cast<QObject*>(v), emscripten::return_value_policy::reference());
    }

    qDebug() << "qvariant_to_val" << v << v.metaType() << v.typeId();

    return emscripten::val::undefined();
}

std::string type_name(emscripten::val val)
{
    std::array<const char*, 4> chunks = {"$$", "ptrType", "registeredClass", "name"};

    for ( auto chunk : chunks )
    {
        val = val[chunk];
        if ( val.isUndefined() )
            return {};
    }
    return val.as<std::string>();
}

QVariant qvariant_from_val(const emscripten::val& val)
{
    if ( val.isNull() || val.isUndefined() )
        return {};

    if ( val.isNumber() )
        return QVariant(val.as<qreal>());

    if ( val.isString() )
        return QVariant(val.as<QString>());

    if ( val.isArray() )
    {
        QVariantList arr;
        auto length = val["length"].as<int>();
        for ( int i = 0; i < length; i++ )
            arr.push_back(qvariant_from_val(val[i]));
        return QVariant(arr);
    }

    std::string name = type_name(val);

    if ( !name.empty() )
    {
        if ( QObject* ptr = val.as<QObject*>(emscripten::allow_raw_pointers()) )
        {
            return QVariant::fromValue(ptr);
        }
    }
    else
    {
        if ( is_byte_array(val) )
        {
            return QVariant(bytearray_from_val(val));
        }
        else if ( val.hasOwnProperty("red") )
        {
            return QVariant::fromValue(QColor(
                val["red"].as<int>(),
                val["green"].as<int>(),
                val["blue"].as<int>(),
                val["alpha"].as<int>()
            ));
        }
        else if ( val.hasOwnProperty("width") )
        {
            return QVariant::fromValue(QSizeF(
                val["width"].as<double>(),
                val["height"].as<double>()
            ));
        }
        else if ( val.hasOwnProperty("x") )
        {
            if ( val.hasOwnProperty("_v") )
                return QVariant::fromValue(QVector2D(
                    val["x"].as<double>(),
                    val["y"].as<double>()
                ));
            return QVariant::fromValue(QPointF(
                val["x"].as<double>(),
                val["y"].as<double>()
            ));
        }
        else if ( val.hasOwnProperty("value") )
        {
            return QVariant(val["value"].as<int>());
        }
    }

    return {};
}

} // namespace glaxnimate::js

namespace emscripten::internal {

#define BIND_OVERLOAD(T) \
template <> struct TypeID<const T&> : public TypeID<T> {}; \
template <> struct BindingType<const T&> : public BindingType<T> {};



template<>
struct TypeID<QString>
{
    static constexpr TYPEID get()
    {
        return TypeID<std::string>::get();
    }
};

template<>
struct BindingType<QString>
{
    typedef typename BindingType<std::string>::WireType WireType;

    static WireType toWireType(const QString& str, rvp::default_tag tag)
    {
        return BindingType<std::string>::toWireType(str.toStdString(), tag);
    }

    static QString fromWireType(WireType v)
    {
        return QString::fromStdString(BindingType<std::string>::fromWireType(v));
    }
};

BIND_OVERLOAD(QString)

template <>
struct TypeID<QVariant>
{
    static constexpr TYPEID get()
    {
        return TypeID<val>::get(); // Tell emscripten: wire type is a JS val
    }
};

template <>
struct BindingType<QVariant>
{
    // using WireType = EM_VAL; // The raw handle type underlying emscripten::val
    typedef typename BindingType<val>::WireType WireType;

    static WireType toWireType(const QVariant& obj, rvp::default_tag)
    {
        return glaxnimate::js::qvariant_to_val(obj).release_ownership();
    }

    static QVariant fromWireType(WireType handle)
    {
        return glaxnimate::js::qvariant_from_val(val::take_ownership(handle));
    }
};

BIND_OVERLOAD(QVariant)



template<>
struct TypeID<QByteArray>
{
    static constexpr TYPEID get()
    {
        return TypeID<val>::get();
    }
};

template<>
struct BindingType<QByteArray>
{
    typedef EM_VAL WireType;

    static WireType toWireType(const QByteArray& arr, rvp::default_tag)
    {
        return glaxnimate::js::bytearray_to_val(arr, true).release_ownership();
    }

    static QByteArray fromWireType(WireType handle)
    {
        return glaxnimate::js::bytearray_from_val(val::take_ownership(handle));
    }
};

BIND_OVERLOAD(QByteArray)


} // namespace emscripten::internal

namespace glaxnimate::js {

template<class... Args> struct Emclass;
template<class CppClass> struct Emclass<CppClass> { using type = emscripten::class_<CppClass>; };
template<class CppClass, class Base> struct Emclass<CppClass, Base> { using type = emscripten::class_<CppClass, emscripten::base<Base>>; };
template<class... Args> using emclass_t = typename Emclass<Args...>::type;

template<class CppClass, class... Args>
emclass_t<CppClass, Args...> declare_from_meta()
{
    const QMetaObject& meta = CppClass::staticMetaObject;
    const char* name = meta.className();
    const char* clean_name = std::strrchr(name, ':');
    if ( clean_name == nullptr )
        clean_name = name;
    else
        clean_name++;

    return emclass_t<CppClass, Args...> (clean_name);
}

} // namespace glaxnimate::js

// Hack using emscripten internals to allow settings class static properties that aren't pointers
namespace emscripten {
namespace internal {

template<typename GetterReturnType>
struct GetterPolicy<std::function<GetterReturnType()>> {
    typedef GetterReturnType ReturnType;
    typedef std::function<GetterReturnType()> Context;

    typedef internal::BindingType<ReturnType> Binding;
    typedef typename Binding::WireType WireType;

    template<typename ClassType, typename ReturnPolicy>
    static WireType get(const Context& context) {
        return Binding::toWireType(context(), ReturnPolicy{});
    }

    static void* getContext(const Context& context) {
        return internal::getContext(context);
    }
};
} // namespace internal
template<
    class ClassType,
    class FieldType,
    typename PropertyType = internal::DeduceArgumentsTag,
    typename... Policies
>
void class_static_property(const char* name, FieldType value, Policies...)
{
    using namespace internal;

    using Getter = std::function<FieldType()>;
    Getter getter([value]{return value;});

    typedef GetterPolicy<
            typename std::conditional<std::is_same<PropertyType, internal::DeduceArgumentsTag>::value,
                                                   Getter,
                                                   PropertyTag<Getter, PropertyType>>::type> GP;
    using ReturnPolicy = typename GetReturnValuePolicy<typename GP::ReturnType>::tag;
    auto gter = &GP::template get<ClassType, ReturnPolicy>;

    _embind_register_class_class_property(
        TypeID<ClassType>::get(),
        name,
        TypeID<FieldType>::get(),
        GP::getContext(getter),
        getSignature(gter),
        reinterpret_cast<GenericFunction>(gter),
        0,
        0);
}
} // namespace emscripten
