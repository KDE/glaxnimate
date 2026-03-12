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


namespace emscripten {

// Why the heck does this work?
// newer version of embind have policies in val::vall but we are restricted by Qt
template<class T, class... Policies>
emscripten::val val_ptr(T* ptr, Policies...)
{
    return emscripten::val(ptr);

}

} // namespace emscripten

namespace glaxnimate::js {


template<class... Enums>
struct enums;

template<class EnumT, class... Others>
struct enums<EnumT, Others...>
    : public enums<Others...>
{
    /*void process(py::handle& scope, std::vector<PyEnumInfo>& out)
    {
        out.push_back(register_enum<EnumT>(QMetaEnum::fromType<EnumT>(), scope));
        enums<Others...>::process(scope, out);
    }*/
};

template<>
struct enums<>
{
    // void process(py::handle&, std::vector<PyEnumInfo>&) {}
};

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
    }

    auto meta_obj = v.metaType().metaObject();
    if ( meta_obj )
    {
        return emscripten::val_ptr(qvariant_cast<QObject*>(v), emscripten::return_value_policy::reference());
    }

    return emscripten::val::undefined();
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
    }

    if ( QObject* ptr = val.as<QObject*>(emscripten::allow_raw_pointers()) )
    {
        return QVariant::fromValue(ptr);
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

template<class CppClass, class... Args, class... Enums>
emclass_t<CppClass, Args...>& register_from_meta(emclass_t<CppClass, Args...>& reg, enums<Enums...> reg_enums = {})
{
    const QMetaObject& meta = CppClass::staticMetaObject;

    for ( int i = meta.propertyOffset(); i < meta.propertyCount(); i++ )
    {
        // PyPropertyInfo pyprop = register_property(meta.property(i), meta);
        // if ( pyprop.name )
            // reg.def_property(pyprop.name, pyprop.get, pyprop.set, "");

        auto prop = meta.property(i);
        if ( !prop.isScriptable() )
            continue;

        /*int meta_type = prop.typeId();
        qDebug() << meta.className() << i << prop.name() << prop.metaType() << meta_type;
        if ( meta_type >= QMetaType::User )
        {
            if ( QMetaType(meta_type).flags() & QMetaType::IsEnumeration )
            {
                // TODO
            }
            else
            {
                reg.property(prop.name(), std::function<QObject*(const CppClass&)>([prop](const CppClass& self) {
                    return qvariant_cast<QObject*>(prop.read(&self));
                }), emscripten::return_value_policy::reference());
            }
        }
        else*/
        {
            reg.property(prop.name(), std::function<emscripten::val(const CppClass&)>([prop](const CppClass& self) {
                return qvariant_to_val(prop.read(&self));
            }));
        }
    }
/*
    for ( int i = meta.methodOffset(); i < meta.methodCount(); i++ )
    {
        PyMethodInfo pymeth = register_method(meta.method(i), reg, meta);
        if ( pymeth.name )
            reg.attr(pymeth.name) = pymeth.method;
    }
*//*
    if ( meta.classInfoOffset() < meta.classInfoCount() )
    {
        emscripten::dict classinfo;

        for ( int i = meta.classInfoOffset(); i < meta.classInfoCount(); i++ )
        {
            auto info = meta.classInfo(i);
            classinfo[info.name()] = info.value();
        }

        reg.attr("__classinfo__") = classinfo;
    }

*/
    /*std::vector<PyEnumInfo> enum_info;
    reg_enums.process(reg, enum_info);
    for ( const auto& info : enum_info )
        reg.attr(info.name) = info.enum_handle;*/
    return reg;
}

template<class CppClass, class... Args, class... Enums>
emclass_t<CppClass, Args...> register_from_meta(enums<Enums...> reg_enums = {})
{
    emclass_t<CppClass, Args...> reg = declare_from_meta<CppClass, Args...>();

    register_from_meta<CppClass, Args...>(reg, reg_enums);
    return reg;
}

template<class CppClass, class... Args, class... Enums>
emclass_t<CppClass, Args...> register_constructible(enums<Enums...> reg_enums = {})
{
    // TODO make constructible?
    return register_from_meta<CppClass, Args...>(reg_enums);
}


} // namespace glaxnimate::js
