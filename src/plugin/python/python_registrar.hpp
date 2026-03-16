/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "casters.hpp"
#include "glaxnimate/script/registrar_common.hpp"
#include "glaxnimate/script/register_impl.hpp"
#include "glaxnimate/script/glaxnimate_model.hpp"

namespace glaxnimate::plugin::python {
namespace py = pybind11;
using namespace glaxnimate::script;
static constexpr auto no_own = py::return_value_policy::automatic_reference;


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

template<class CppType>
struct ConvertReturn
{
    static QVariant do_the_thing(script::ArgumentBuffer& buf)
    {
        return qvariant_from_cpp(buf.return_value<CppType>());
    }
};

template<class CppType>
struct RegisterProperty
{
    template<class Class>
    static bool do_the_thing(const QMetaProperty& prop, Class& reg)
    {
        const char* name = prop.name();
        std::string sig = "Type: " + fix_type(prop.typeName());
        auto get = py::cpp_function(
            [prop](const QObject* o) { return qvariant_to_cpp<CppType>(prop.read(o)); },
            py::return_value_policy::automatic_reference,
            sig.c_str()
        );

        py::cpp_function set;
        if ( prop.isWritable() )
            set = py::cpp_function([prop](QObject* o, const CppType& v) {
                prop.write(o, qvariant_from_cpp<CppType>(v));
            });


        reg.def_property(name, get, set, "");
        return true;
    }
};



template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class AddShapeClass : public AddShapeBase<Owner, PropT, ItemT>
{
public:
    using AddShapeBase<Owner, PropT, ItemT>::AddShapeBase;

    ItemT* operator() (Owner* owner, const py::object& cls, int index = -1) const
    {
        pybind11::detail::type_caster<QString> cast;
        cast.load(cls.attr("__name__"), true);
        return this->create(owner->document(), owner->*(this->ptr), cast, index);
    }
};

struct PythonRegistrar
{
    using namespace_ = py::handle;

    template<class CppClass, class... Args>
    using class_ = py::class_<CppClass, Args...>;

    using module = py::module;

    static module submodule(module& parent, const char* name, const char* docs = "")
    {
        return parent.def_submodule(name, docs);
    }

    template<class CppClass, class... Args>
    static class_<CppClass, Args...> define_class(const module& mod, const char* name)
    {
        return class_<CppClass, Args...>(mod, name);
    }

    template<class Class>
    static void set_class_static(Class& reg, const char* name, const QVariant& value)
    {
        reg.attr(name) = value;
    }

    template<class Class>
    static void register_method(const QMetaMethod& meth, Class& handle)
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
        }
        signature += ") -> ";
        signature += fix_type(meth.typeName());

        handle.def(meth.name().constData(),
            [meth](QObject* o, py::args args) -> QVariant
            {
                int len = py::len(args);
                if ( len > 9 )
                    throw pybind11::value_error("Invalid argument count");

                try {
                    ArgumentBuffer argbuf(meth);


                    script::type_dispatch<AllocateReturn, bool>(meth.returnType(), argbuf, meth.typeName());

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

                    return script::type_dispatch<ConvertReturn, QVariant>(meth.returnType(), argbuf);

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
    }


    template<class Class>
    static bool register_property(const QMetaProperty& prop, Class& handle)
    {
        return type_dispatch<RegisterProperty, bool>(prop.userType(), prop, handle);
    }


    template<class EnumT, class Class>
    static void register_enum(const QMetaEnum& meta, Class& scope)
    {
        py::enum_<EnumT> pyenum(scope, meta.name());
        for ( int i = 0; i < meta.keyCount(); i++ )
            pyenum.value(meta.key(i), EnumT(meta.value(i)));

        // scope.attr(meta.name()) = pyenum;
    }

// glaxnimate-specific stuff
    template<
        class PyClass,
        class PropT = glaxnimate::model::ObjectListProperty<model::ShapeElement>,
        class Owner = typename PyClass::type,
        class ItemT = typename PropT::value_type
    >
    static void define_add_shape(PyClass& cls, PropT Owner::* prop = &Owner::shapes, const std::string& name = "add_shape")
    {
        cls.def(
                name.c_str(),
                AddShapeName<Owner, PropT, ItemT>(prop),
                no_own,
                "Adds a shape from its class name",
                py::arg("type_name"),
                py::arg("index") = -1
            )
            .def(
                name.c_str(),
                AddShapeClone<Owner, PropT, ItemT>(prop),
                no_own,
                "Adds a shape, note that the input object is cloned, and the clone is returned. The document will have ownership over the clone.",
                py::arg("object"),
                py::arg("index") = -1
            )
            .def(
                name.c_str(),
                AddShapeClass<Owner, PropT, ItemT>(prop),
                no_own,
                "Adds a shape from its class",
                py::arg("cls"),
                py::arg("index") = -1
            )
        ;
    }

    template<class CppClass, class... Args>
    static void glaxnimate_constructible(class_<CppClass, Args...>& reg)
    {
        reg.def(py::init([](model::Document* doc) -> std::unique_ptr<CppClass> {
            if ( !doc )
                return {};
            return std::make_unique<CppClass>(doc);
        }));
    }
};

} // namespace glaxnimate::plugin::python
