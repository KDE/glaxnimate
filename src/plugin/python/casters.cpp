/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <utility>

#include "glaxnimate/log/log.hpp"
#include "glaxnimate/script/register_impl.hpp"
#include "casters.hpp"


using namespace glaxnimate::script;
namespace py = pybind11;


namespace glaxnimate::plugin::python {

template<int i>
bool qvariant_type_caster_load_impl(QVariant& into, const pybind11::handle& src)
{
    auto caster = pybind11::detail::make_caster<meta_2_cpp<i>>();
    if ( caster.load(src, false) )
    {
        into = QVariant::fromValue(pybind11::detail::cast_op<meta_2_cpp<i>>(caster));
        return true;
    }
    return false;
}

template<>
bool qvariant_type_caster_load_impl<QMetaType::QVariant>(QVariant&, const pybind11::handle&) { return false; }

template<int... i>
bool qvariant_type_caster_load(QVariant& into, const pybind11::handle& src, std::integer_sequence<int, i...>)
{
    return (qvariant_type_caster_load_impl<i>(into, src)||...);
}

template<int i>
bool qvariant_type_caster_cast_impl(
    pybind11::handle& into, const QVariant& src,
    pybind11::return_value_policy policy, const pybind11::handle& parent)
{
    if ( src.userType() == i )
    {
        into = pybind11::detail::make_caster<meta_2_cpp<i>>::cast(src.value<meta_2_cpp<i>>(), policy, parent);
        return true;
    }
    return false;
}

template<>
bool qvariant_type_caster_cast_impl<QMetaType::QVariant>(
    pybind11::handle&, const QVariant&, pybind11::return_value_policy, const pybind11::handle&)
{
    return false;
}

template<int... i>
bool qvariant_type_caster_cast(
    pybind11::handle& into,
    const QVariant& src,
    pybind11::return_value_policy policy,
    const pybind11::handle& parent,
    std::integer_sequence<int, i...>
)
{
    return (qvariant_type_caster_cast_impl<i>(into, src, policy, parent)||...);
}

} // namespace glaxnimate::plugin::python

using namespace glaxnimate::plugin::python;




std::map<int, pybind11::detail::type_caster<QVariant>::CustomConverter> pybind11::detail::type_caster<QVariant>::custom_converters;

bool pybind11::detail::type_caster<QVariant>::load(handle src, bool)
{
    if ( src.ptr() == Py_None )
    {
        value = QVariant();
        return true;
    }

    if ( qvariant_type_caster_load(value, src, supported_types()) )
        return true;

    for ( const auto& p : custom_converters )
        if ( p.second.load(src, value) )
            return true;
    return false;
}

pybind11::handle pybind11::detail::type_caster<QVariant>::cast(QVariant src, return_value_policy policy, handle parent)
{
    if ( src.isNull() )
        return pybind11::none();

    policy = py::return_value_policy::automatic_reference;

    int meta_type = src.userType();
    if ( meta_type >= QMetaType::User )
    {
        if ( QMetaType(meta_type).flags() & QMetaType::IsEnumeration )
            return pybind11::detail::make_caster<int>::cast(src.value<int>(), policy, parent);
        else if ( meta_type == qMetaTypeId<QGradientStops>() )
            return pybind11::detail::make_caster<QGradientStops>::cast(src.value<QGradientStops>(), policy, parent);

        auto it = custom_converters.find(meta_type);
        if ( it != custom_converters.end() )
            return it->second.cast(src, policy, parent);
        return pybind11::detail::make_caster<QObject*>::cast(src.value<QObject*>(), policy, parent);
    }

    pybind11::handle ret;
    qvariant_type_caster_cast(ret, src, policy, parent, supported_types());
    return ret;
}


bool pybind11::detail::type_caster<QUuid>::load(handle src, bool ic)
{
    if ( isinstance(src, pybind11::module_::import("uuid").attr("UUID")) )
        src = py::str(src);
    type_caster<QString> stdc;
    if ( stdc.load(src, ic) )
    {
        value = QUuid::fromString((const QString &)stdc);
        return true;
    }
    return false;
}

pybind11::handle pybind11::detail::type_caster<QUuid>::cast(QUuid src, return_value_policy policy, handle parent)
{
    auto str = type_caster<QString>::cast(src.toString(), policy, parent);
    return pybind11::module_::import("uuid").attr("UUID")(str).release();
}



bool pybind11::detail::type_caster<QImage>::load(handle src, bool)
{
    if ( !isinstance(src, pybind11::module_::import("PIL.Image").attr("Image")) )
        return false;

    py::object obj = py::reinterpret_borrow<py::object>(src);
    std::string mode = obj.attr("mode").cast<std::string>();
    QImage::Format format;
    if ( mode == "RGBA" )
    {
        format = QImage::Format_RGBA8888;
    }
    else if ( mode == "RGB" )
    {
        format = QImage::Format_RGB888;
    }
    else if ( mode == "RGBa" )
    {
        format = QImage::Format_RGBA8888_Premultiplied;
    }
    else if ( mode == "RGBX" )
    {
        format = QImage::Format_RGBX8888;
    }
    else
    {
        format = QImage::Format_RGBA8888;
        obj = obj.attr("convert")("RGBA");
    }

    std::string data = obj.attr("tobytes")().cast<std::string>();

    int width = obj.attr("width").cast<int>();
    int height = obj.attr("height").cast<int>();
    value = QImage(width, height, format);
    if ( data.size() != std::size_t(value.sizeInBytes()) )
        return false;
    std::memcpy(value.bits(), data.data(), data.size());
    return true;
}

pybind11::handle pybind11::detail::type_caster<QImage>::cast(QImage src, return_value_policy, handle)
{
    auto mod = pybind11::module_::import("PIL.Image");
    auto frombytes = mod.attr("frombytes");

    const char* mode;

    switch ( src.format() )
    {
        case QImage::Format_Invalid:
            return mod.attr("Image")().release();
        case QImage::Format_RGB888:
            mode = "RGB";
            break;
        case QImage::Format_RGBA8888:
            mode = "RGBA";
            break;
        case QImage::Format_RGBA8888_Premultiplied:
            mode = "RGBa";
            break;
        case QImage::Format_RGBX8888:
            mode = "RGBX";
            break;
        default:
            mode = "RGBA";
            src = src.convertToFormat(QImage::Format_RGBA8888);
            break;
    }

    py::tuple size(2);
    size[0] = py::int_(src.width());
    size[1] = py::int_(src.height());

    auto image = frombytes(
        mode,
        size,
        py::bytes((const char*)src.bits(), src.sizeInBytes())
    );

    return image.release();
}
