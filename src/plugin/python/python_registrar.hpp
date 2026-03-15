/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "casters.hpp"
#include "glaxnimate/script/registrar_common.hpp"
#include "glaxnimate/script/register_impl.hpp"

namespace glaxnimate::plugin::python {
namespace py = pybind11;
using namespace glaxnimate::script;


std::string fix_type(QByteArray ba);

template<class CppType>
struct ConvertArgument
{
    static bool do_the_thing(const py::handle& val, ArgumentBuffer& buf)
    {
        buf.allocate<CppType>(val.cast<CppType>());
        return true;
    }
};


template<class ReturnType>
struct RegisterMethod
{
    template<class T>
    static bool do_the_thing(const QMetaMethod& meth, T& handle)
    {
        std::string signature = "Signature:\n";
        signature += meth.name().toStdString();
        signature += "(self";
        auto names = meth.parameterNames();
        auto types = meth.parameterTypes();
        for ( int i = 0; i < meth.parameterCount(); i++ )
        {
            signature += ", ";
            signature += names[i].toStdString();
            signature += ": ";
            signature += fix_type(types[i]);

            if ( meth.parameterType(i) == QMetaType::UnknownType )
            {
                auto cls = py::str(handle.attr("__name__"));
                log::LogStream("Python", "", log::Error)
                    << "Invalid parameter" << QString::fromStdString(cls) << "::" << meth.name()
                    << i << names[i]
                    << "of type" << meth.parameterType(i) << types[i];
                return false;
            }
        }
        signature += ") -> ";
        signature += fix_type(meth.typeName());

        handle.def(meth.name().constData(),
            [meth](QObject* o, py::args args) -> ReturnType
            {
                int len = py::len(args);
                if ( len > 9 )
                    throw pybind11::value_error("Invalid argument count");

                try {
                    ArgumentBuffer argbuf(meth);

                    argbuf.allocate_return_type<ReturnType>(meth.typeName());

                    for ( int i = 0; i < len; i++ )
                    {
                        if ( !type_dispatch<ConvertArgument, bool>(meth.parameterType(i), args[i], argbuf) )
                            throw pybind11::value_error("Invalid argument");
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
                        throw pybind11::value_error("Invalid method invocation");
                    return argbuf.return_value<ReturnType>();

                } catch ( const ScriptError& err ) {
                    throw py::type_error(err.what());
                }
            },
            py::name(meth.name().constData()),
            py::is_method(handle),
            py::sibling(py::getattr(handle, meth.name().constData(), py::none())),
            py::return_value_policy::automatic_reference,
            signature.c_str()
        );

        return true;
    }
};

struct PythonRegistrar
{
    /*
    using val_type = py::handle;

    template<class T>
    static T val_cast(const val_type& val) { return val.cast<T>(); }

    template<class CppClass, class... Args>
    using class_ = py::class_<CppClass, Args...>;
*/

    template<class Class>
    static void register_method(const QMetaMethod& meth, Class& handle)
    {
        type_dispatch_maybe_void<RegisterMethod, bool>(meth.returnType(), meth, handle);
    }
};

} // namespace glaxnimate::plugin::python
