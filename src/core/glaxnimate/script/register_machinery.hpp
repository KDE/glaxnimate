/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "registrar_common.hpp"
#include "glaxnimate/script/register_impl.hpp"

#include <QMetaMethod>
#include <QDebug>

namespace glaxnimate::script {

template<class Reg, class Class>
void register_method(const QMetaMethod& meth, const QMetaObject& cls, Class& handle)
{
    if ( meth.access() != QMetaMethod::Public )
        return;
    if ( meth.methodType() != QMetaMethod::Method && meth.methodType() != QMetaMethod::Slot )
        return;

    if ( meth.parameterCount() > 9 )
    {
        log::LogStream("Scripting", "", log::Error) << "Too many arguments for method " << cls.className() << "::" << meth.name() << ": " << meth.parameterCount();
        return;
    }

    for ( int i = 0; i < meth.parameterCount(); i++ )
    {
        if ( meth.parameterType(i) == QMetaType::UnknownType )
        {
            log::LogStream("Scripting", "", log::Error)
                << "Invalid parameter type"
                << cls.className() << "::" << meth.name()
                << i << meth.parameterNames()[i];
            return;
        }
    }

    Reg::register_method(meth, handle);
}

template<class Reg, class Class>
void register_property(const QMetaProperty& prop, const QMetaObject& meta, Class& handle)
{
    if ( !prop.isScriptable() )
        return;

    if ( !Reg::register_property(prop, handle) )
        log::LogStream("Python", "", log::Error) << "Invalid property" << meta.className() << "::" << prop.name() << "of type" << prop.userType() << prop.typeName();
}


template<class... Enums>
struct enums;

template<class EnumT, class... Others>
struct enums<EnumT, Others...>
    : public enums<Others...>
{
    template<class Reg, class Class>
    void process(Class& scope)
    {
        Reg::template register_enum<EnumT>(QMetaEnum::fromType<EnumT>(), scope);
        enums<Others...>::template process<Reg>(scope);
    }
};

template<>
struct enums<>
{
    template<class Reg, class Class>
    void process(Class&) {}
};


template<class Reg, class CppClass, class... Args>
typename Reg::template class_<CppClass, Args...> declare_from_meta(const typename Reg::module& scope)
{
    const QMetaObject& meta = CppClass::staticMetaObject;
    const char* name = meta.className();
    const char* clean_name = std::strrchr(name, ':');
    if ( clean_name == nullptr )
        clean_name = name;
    else
        clean_name++;

    return Reg::template define_class<CppClass, Args...> (scope, clean_name);
}

template<class Reg, class CppClass, class... Args, class... Enums>
typename Reg::template class_<CppClass, Args...>& populate_from_meta(
    typename Reg::template class_<CppClass, Args...>& reg, enums<Enums...> reg_enums = {})
{
    const QMetaObject& meta = CppClass::staticMetaObject;

    for ( int i = meta.propertyOffset(); i < meta.propertyCount(); i++ )
    {
        register_property<Reg>(meta.property(i), meta, reg);
    }

    for ( int i = meta.methodOffset(); i < meta.methodCount(); i++ )
    {
        register_method<Reg>(meta.method(i), meta, reg);
    }

    if ( meta.classInfoOffset() < meta.classInfoCount() )
    {
        QVariantMap classinfo;

        for ( int i = meta.classInfoOffset(); i < meta.classInfoCount(); i++ )
        {
            auto info = meta.classInfo(i);
            classinfo[info.name()] = info.value();
        }

        Reg::set_class_static(reg, "__classinfo__", QVariant(classinfo));
    }

    reg_enums.template process<Reg>(reg);

    return reg;
}

template<class Reg, class CppClass, class... Args, class... Enums>
typename Reg::template class_<CppClass, Args...> register_from_meta(
    const typename Reg::module& scope,
    enums<Enums...> reg_enums = {}
)
{
    auto reg = declare_from_meta<Reg, CppClass, Args...>(scope);
    populate_from_meta<Reg, CppClass, Args...>(reg, reg_enums);
    return reg;
}


template<class Reg, class Cls, class... Args, class... FwArgs>
auto register_constructible(const typename Reg::module& module, FwArgs&&... args)
{
    auto reg = register_from_meta<Reg, Cls, Args...>(module, std::forward<FwArgs>(args)...);
    Reg::template glaxnimate_constructible<Cls, Args...>(reg);
    return reg;
}

namespace detail {

template<class T>
QString qdebug_operator_to_string(const T& o)
{
    QString data;
    QDebug(&data) << o;
    return data;
}

} // namespace detail

template<class T>
auto qdebug_operator_to_string()
{
    return &detail::qdebug_operator_to_string<T>;
}

} // namespace glaxnimate::script
