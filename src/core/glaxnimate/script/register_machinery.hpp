/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "registrar_common.hpp"
#include "glaxnimate/script/register_impl.hpp"

#include <QMetaMethod>

#if __GNUC__ >= 4
#   define SCRIPT_HIDDEN  __attribute__ ((visibility ("hidden")))
#else
#   define SCRIPT_HIDDEN
#endif


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


} // namespace glaxnimate::script
