/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cstring>

#include <QMetaProperty>
#include <QMetaEnum>
#include <QDebug>

#include "casters.hpp"
#include "glaxnimate/script/register_machinery.hpp"
#include "python_registrar.hpp"

namespace py = pybind11;

namespace glaxnimate::plugin::python {

struct SCRIPT_HIDDEN PyPropertyInfo
{
    const char* name = nullptr;
    py::cpp_function get;
    py::cpp_function set;
};

struct SCRIPT_HIDDEN PyEnumInfo
{
    const char* name = nullptr;
    py::handle enum_handle;
};

PyPropertyInfo register_property(const QMetaProperty& prop, const QMetaObject& cls);

template<class EnumT>
PyEnumInfo register_enum(const QMetaEnum& meta, py::handle& scope)
{
    py::enum_<EnumT> pyenum(scope, meta.name());
    for ( int i = 0; i < meta.keyCount(); i++ )
        pyenum.value(meta.key(i), EnumT(meta.value(i)));

    return {meta.name(), pyenum};
}


template<class... Enums>
struct enums;

template<class EnumT, class... Others>
struct enums<EnumT, Others...>
    : public enums<Others...>
{
    void process(py::handle& scope, std::vector<PyEnumInfo>& out)
    {
        out.push_back(register_enum<EnumT>(QMetaEnum::fromType<EnumT>(), scope));
        enums<Others...>::process(scope, out);
    }
};

template<>
struct enums<>
{
    void process(py::handle&, std::vector<PyEnumInfo>&) {}
};


template<class CppClass, class... Args>
py::class_<CppClass, Args...> declare_from_meta(py::handle scope)
{
    const QMetaObject& meta = CppClass::staticMetaObject;
    const char* name = meta.className();
    const char* clean_name = std::strrchr(name, ':');
    if ( clean_name == nullptr )
        clean_name = name;
    else
        clean_name++;

    return py::class_<CppClass, Args...> (scope, clean_name);
}

template<class CppClass, class... Args, class... Enums>
py::class_<CppClass, Args...> register_from_meta(py::handle scope, enums<Enums...> reg_enums = {})
{
    py::class_<CppClass, Args...> reg = declare_from_meta<CppClass, Args...>(scope);
    register_from_meta(reg, reg_enums);
    return reg;
}

template<class CppClass, class... Args, class... Enums>
py::class_<CppClass, Args...>& register_from_meta(py::class_<CppClass, Args...>& reg, enums<Enums...> reg_enums = {})
{
    using Reg = PythonRegistrar;
    const QMetaObject& meta = CppClass::staticMetaObject;

    for ( int i = meta.propertyOffset(); i < meta.propertyCount(); i++ )
    {
        PyPropertyInfo pyprop = register_property(meta.property(i), meta);
        if ( pyprop.name )
            reg.def_property(pyprop.name, pyprop.get, pyprop.set, "");
    }

    for ( int i = meta.methodOffset(); i < meta.methodCount(); i++ )
    {
        register_method<Reg>(meta.method(i), reg, meta);
    }

    if ( meta.classInfoOffset() < meta.classInfoCount() )
    {
        py::dict classinfo;

        for ( int i = meta.classInfoOffset(); i < meta.classInfoCount(); i++ )
        {
            auto info = meta.classInfo(i);
            classinfo[info.name()] = info.value();
        }

        reg.attr("__classinfo__") = classinfo;
    }

    std::vector<PyEnumInfo> enum_info;
    reg_enums.process(reg, enum_info);
    for ( const auto& info : enum_info )
        reg.attr(info.name) = info.enum_handle;

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

} // namespace glaxnimate::plugin::python
