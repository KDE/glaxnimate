/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <emscripten/console.h>
#include "register_js.hpp"
#include "glaxnimate/script/registrar_common.hpp"
#include "glaxnimate/script/register_impl.hpp"

namespace glaxnimate::js {


template<class CppType>
struct ConvertArgument
{
    static bool do_the_thing(const QVariant& val, script::ArgumentBuffer& buf)
    {
        buf.allocate<CppType>(qvariant_cast<CppType>(val));
        return true;
    }
};



template<class CppType>
struct ConvertReturn
{
    static emscripten::val do_the_thing(script::ArgumentBuffer& buf)
    {
        return qvariant_to_val(QVariant::fromValue(buf.return_value<CppType>()));
    }
};

template<class... Args> struct Emclass;
template<class CppClass> struct Emclass<CppClass> { using type = emscripten::class_<CppClass>; };
template<class CppClass, class Base> struct Emclass<CppClass, Base> { using type = emscripten::class_<CppClass, emscripten::base<Base>>; };


struct JsRegistrar
{
    template<class CppClass, class... Args>
    using class_ = typename Emclass<CppClass, Args...>::type;

    using module = int;
    static module submodule(module&, const char*, const char* = "")
    {
        return 0;
    }

    template<class CppClass, class... Args>
    static class_<CppClass, Args...> define_class(const module&, const char* name)
    {
        return class_<CppClass, Args...>(name);
    }

    template<class Class>
    static void register_method(const QMetaMethod& meth, Class& handle)
    {

        handle.function(meth.name().constData(),
            fn([meth](
                const emscripten::val& self,
                const std::optional<emscripten::val>& arg1,
                const std::optional<emscripten::val>& arg2,
                const std::optional<emscripten::val>& arg3,
                const std::optional<emscripten::val>& arg4,
                const std::optional<emscripten::val>& arg5,
                const std::optional<emscripten::val>& arg6,
                const std::optional<emscripten::val>& arg7,
                const std::optional<emscripten::val>& arg8,
                const std::optional<emscripten::val>& arg9
            ) -> emscripten::val
            {
                script::ArgumentBuffer argbuf(meth);

                script::type_dispatch<script::AllocateReturn, bool>(meth.returnType(), argbuf, meth.typeName());

                QObject* o = qvariant_cast<QObject*>(self.as<QVariant>());
                if ( !o )
                {
                    emscripten_console_error("Invalid this argument");
                    return {};
                }

                std::array<const std::optional<emscripten::val>*, 9> args = {
                    &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9
                };

                for ( std::size_t i = 0; i < args.size(); i++ )
                {
                    QVariant v;
                    if ( *args[i] )
                        v = (*args[i])->as<QVariant>();
                    if ( !script::type_dispatch<ConvertArgument, bool>(meth.parameterType(i), v, argbuf) )
                    {
                        emscripten_console_error("Invalid argument");
                        return {};
                    }
                }

                // Calling by name from QMetaObject ensures that default arguments work correctly
                bool ok = QMetaObject::invokeMethod(
                    o,
                    meth.name().constData(),
                    Qt::DirectConnection,
                    argbuf.return_arg(),
                    argbuf.arg(0),
                    argbuf.arg(1),
                    argbuf.arg(2),
                    argbuf.arg(3),
                    argbuf.arg(4),
                    argbuf.arg(5),
                    argbuf.arg(6),
                    argbuf.arg(7),
                    argbuf.arg(8),
                    argbuf.arg(9)
                );
                if ( !ok )
                {
                    emscripten_console_error("Invalid method invocation");
                    return {};
                }
                return script::type_dispatch<ConvertReturn, emscripten::val>(meth.returnType(), argbuf);
            }),
            emscripten::return_value_policy::reference()
        );
    }


    template<class Class>
    static bool register_property(const QMetaProperty& prop, Class& reg)
    {
        auto getter = fn([prop](const QObject& self) {
            return qvariant_to_val(prop.read(&self));
        });
        if ( prop.isWritable() )
            reg.property(prop.name(), std::move(getter), fn(
                [prop]( QObject& self, const QVariant& val) {
                    prop.write(&self, val);
                }
            ));
        else
            reg.property(prop.name(), std::move(getter));
        return true;
    }


    template<class Class>
    static void set_class_static(Class&, const char* name, const QVariant& value)
    {
        class_static_property<typename Class::class_type>(name, value);
    }

    template<class EnumT, class Class>
    static void register_enum(const QMetaEnum& meta, Class& scope)
    {
        QVariantMap static_val;
        emscripten::enum_<EnumT> jsenum(meta.name());
        for ( int i = 0; i < meta.keyCount(); i++ )
        {
            jsenum.value(meta.key(i), EnumT(meta.value(i)));
            static_val[meta.key(i)] = QVariant::fromValue(EnumT(meta.value(i)));
        }

        set_class_static(scope, meta.name(), static_val);
    }
};


template<class Reg, class CppClass, class... Args, class... Enums>
typename Reg::template class_<CppClass, Args...> register_constructible(typename Reg::module& mod, script::enums<Enums...> reg_enums = {})
{
    // TODO make constructible?
    return script::register_from_meta<Reg, CppClass, Args...>(mod, reg_enums);
}

} // namespace glaxnimate::js
