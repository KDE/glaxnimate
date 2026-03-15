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
struct AllocateReturn
{
    static bool do_the_thing(script::ArgumentBuffer& buf, const char* name)
    {
        buf.allocate_return_type<CppType>(name);
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


struct JsRegistrar
{
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

                script::type_dispatch<AllocateReturn, bool>(meth.returnType(), argbuf, meth.typeName());

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
};


template<class CppClass, class... Args, class... Enums>
emclass_t<CppClass, Args...>& register_from_meta(emclass_t<CppClass, Args...>& reg, enums<Enums...> reg_enums = {})
{
    using Reg = JsRegistrar;
    const QMetaObject& meta = CppClass::staticMetaObject;

    for ( int i = meta.propertyOffset(); i < meta.propertyCount(); i++ )
    {
        // PyPropertyInfo pyprop = register_property(meta.property(i), meta);
        // if ( pyprop.name )
            // reg.def_property(pyprop.name, pyprop.get, pyprop.set, "");

        auto prop = meta.property(i);
        if ( !prop.isScriptable() )
            continue;

        // qDebug() << meta.className() << emscripten::internal::TypeID<CppClass>::get() << i << prop.name() << prop.metaType();
        /*int meta_type = prop.typeId();
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
            auto getter = fn([prop](const CppClass& self) {
                return qvariant_to_val(prop.read(&self));
            });
            if ( prop.isWritable() )
                reg.property(prop.name(), std::move(getter), fn(
                    [prop]( CppClass& self, const QVariant& val) {
                        prop.write(&self, val);
                    }
                ));
            else
                reg.property(prop.name(), std::move(getter));
        }
    }

    for ( int i = meta.methodOffset(); i < meta.methodCount(); i++ )
    {
        script::register_method<Reg>(meta.method(i), reg, meta);
    }


    if ( meta.classInfoOffset() < meta.classInfoCount() )
    {
        emscripten::val classinfo = emscripten::val::object();

        for ( int i = meta.classInfoOffset(); i < meta.classInfoCount(); i++ )
        {
            auto info = meta.classInfo(i);
            classinfo.set(info.name(), emscripten::val(info.value()));
        }

        emscripten::class_static_property<CppClass>("__classinfo__", classinfo);
    }


    /*std::vector<PyEnumInfo> enum_info;*/
    reg_enums.process(/*, enum_info*/);
    /*for ( const auto& info : enum_info )
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
