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


template<class ReturnType>
struct RegisterMethod
{
    template<class T>
    static bool do_the_thing(const QMetaMethod& meth, T& handle)
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
            ) -> ReturnType
            {
                script::ArgumentBuffer argbuf(meth);

                argbuf.allocate_return_type<ReturnType>(meth.typeName());

                emscripten::val::global("console").call<void>("log", self);

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
                return argbuf.return_value<ReturnType>();

            }),
            emscripten::return_value_policy::reference()
        );

        return true;
    }
};

struct JsRegistrar
{

    template<class Class>
    static void register_method(const QMetaMethod& meth, Class& handle)
    {
        script::type_dispatch_maybe_void<RegisterMethod, bool>(meth.returnType(), meth, handle);
    }
};

} // namespace glaxnimate::js
