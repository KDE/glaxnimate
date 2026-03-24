/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <pybind11/operators.h>

#include "glaxnimate/model/visitor.hpp"
#include "glaxnimate/model/animation/meta_animatable.hpp"

#include "glaxnimate/command/animation_commands.hpp"
#include "glaxnimate/command/undo_macro_guard.hpp"

#include "glaxnimate/io/glaxnimate/glaxnimate_format.hpp"
#include "glaxnimate/io/raster/raster_format.hpp"
#include "glaxnimate/io/raster/raster_mime.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"
#include "glaxnimate/io/svg/svg_renderer.hpp"
#include "glaxnimate/io/rive/rive_format.hpp"

#include "glaxnimate/script/glaxnimate_model.hpp"


#include "plugin/io.hpp"
#include "glaxnimate/app_info.hpp"

#include "miscdefs.hpp"

using namespace glaxnimate::script;
using namespace glaxnimate::plugin::python;
using namespace glaxnimate;

namespace {

using Reg = PythonRegistrar;

template<class T, class Base=model::AnimatedPropertyBase>
void register_animatable(py::module& m)
{
    std::string name = "AnimatedProperty<";
    name += QMetaType::fromType<T>().name();
    name += ">";
    py::class_<model::AnimatedProperty<T>, Base>(m, name.c_str());


    name = "Keyframe<";
    name += QMetaType::fromType<T>().name();
    name += ">";
    py::class_<model::Keyframe<T>, model::KeyframeBase>(m, name.c_str());
}

static QImage doc_to_image(model::Composition* comp)
{
    return io::raster::RasterMime::to_image({comp});
}

static QByteArray frame_to_svg(model::Composition* comp)
{
    QByteArray data;
    QBuffer file(&data);
    file.open(QIODevice::WriteOnly);

    io::svg::SvgRenderer rend(io::svg::NotAnimated, io::svg::CssFontType::FontFace);
    rend.write_main(comp, comp->document()->current_time());
    rend.write(&file, true);

    return data;
}

void define_io(py::module& m)
{
    py::module io = m.def_submodule("io", "Input/Output utilities");

    py::class_<io::mime::MimeSerializer>(io, "MimeSerializer")
        .def_property_readonly("slug", &io::mime::MimeSerializer::slug)
        .def_property_readonly("name", &io::mime::MimeSerializer::name)
        .def_property_readonly("mime_types", &io::mime::MimeSerializer::mime_types)
        .def("serialize", &io::mime::MimeSerializer::serialize)
    ;

    const char* to_image_docstring = "Renders the current frame to an image";
    py::class_<io::raster::RasterMime, io::mime::MimeSerializer>(io, "RasterMime")
        .def_static("render_frame", &io::raster::RasterMime::to_image, to_image_docstring)
        .def_static("render_frame", &doc_to_image, to_image_docstring)
        .def_static("render_frame", &io::raster::RasterMime::frame_to_image,
                    "Renders the given frame to image",
                    py::arg("node"), py::arg("frame"))
    ;

    using Fac = io::IoRegistry;
    py::class_<Fac, std::unique_ptr<Fac, py::nodelete>>(io, "IoRegistry")
        .def("importers", &Fac::importers, no_own)
        .def("exporters", &Fac::exporters, no_own)
        .def("from_extension", &Fac::from_extension, no_own)
        .def("from_filename", &Fac::from_filename, no_own)
        .def("from_slug", &Fac::from_slug, no_own)
        .def("__getitem__", &Fac::from_slug, no_own)
        .def("serializers", &Fac::serializers, no_own)
        .def("serializer_from_slug", &Fac::serializer_from_slug, no_own)
    ;

    io.attr("registry") = std::unique_ptr<Fac, py::nodelete>(&io::IoRegistry::instance());

    auto import_export = register_from_meta<Reg, io::ImportExport, QObject>(io, enums<io::ImportExport::Direction>{})
        .def("progress_max_changed", &io::ImportExport::progress_max_changed)
        .def("progress", &io::ImportExport::progress)
    ;
    io.attr("Direction") = import_export.attr("Direction");

    register_from_meta<Reg, io::glaxnimate::GlaxnimateFormat, io::ImportExport>(io)
        .attr("instance") = std::unique_ptr<io::glaxnimate::GlaxnimateFormat, py::nodelete>(io::glaxnimate::GlaxnimateFormat::instance())
    ;

    register_from_meta<Reg, io::raster::RasterFormat, io::ImportExport>(io)
        .def_static("render_frame", &io::raster::RasterMime::to_image, to_image_docstring)
        .def_static("render_frame", &doc_to_image, to_image_docstring)
    ;

    register_from_meta<Reg, io::svg::SvgFormat, io::ImportExport>(io)
        .def_static("render_frame", &frame_to_svg, "renders the current frame to SVG")
    ;

    register_from_meta<Reg, plugin::IoFormat, io::ImportExport>(io);


    register_from_meta<Reg, io::rive::RiveFormat, io::ImportExport>(io)
        .def("to_json_data", [](io::rive::RiveFormat& self, const QByteArray& data){
            return self.to_json(data).toJson();
        }, py::arg("binary_data"))
    ;
}


void define_animatable(py::module& m)
{
    py::class_<model::KeyframeTransition> kt(m, "KeyframeTransition");
    Reg::register_enum<model::KeyframeTransition::Descriptive>(QMetaEnum::fromType<model::KeyframeTransition::Descriptive>(), kt);
    kt
        .def(py::init<>())
        .def(py::init<const QPointF&, const QPointF&>())
        .def_property("hold", &model::KeyframeTransition::hold, &model::KeyframeTransition::set_hold)
        .def_property("before", &model::KeyframeTransition::before, &model::KeyframeTransition::set_before)
        .def_property("after", &model::KeyframeTransition::after, &model::KeyframeTransition::set_after)
        .def_property("before_descriptive", &model::KeyframeTransition::before_descriptive, &model::KeyframeTransition::set_before_descriptive)
        .def_property("after_descriptive", &model::KeyframeTransition::after_descriptive, &model::KeyframeTransition::set_after_descriptive)
        .def("lerp_factor", &model::KeyframeTransition::lerp_factor)
        .def("bezier_parameter", &model::KeyframeTransition::bezier_parameter)
    ;

    register_from_meta<Reg, model::KeyframeBase, QObject>(m);
    register_from_meta<Reg, model::AnimatableBase, QObject>(m);
    register_from_meta<Reg, model::AnimatedPropertyBase, model::AnimatableBase>(m)
        .def("set_keyframe", [](model::AnimatedPropertyBase& a, model::FrameTime time, const QVariant& value){
            a.object()->document()->undo_stack().push(
                a.command_add_smooth_keyframe(time, value, true)
            );
            return a.keyframe_at(time);
        }, no_own, py::arg("time"), py::arg("value"))
        .def("remove_keyframe", [](model::AnimatedPropertyBase& a, model::FrameTime time){
            a.object()->document()->undo_stack().push(
                a.command_remove_keyframe(time)
            );
        }, py::arg("time"))
        .def("clear_keyframes", [](model::AnimatedPropertyBase& a){
            a.object()->document()->undo_stack().push(
                a.command_clear_keyframes()
            );
        })
        .def("set_transition", [](model::AnimatedPropertyBase& a, model::FrameTime time, const model::KeyframeTransition& transition){
            a.object()->document()->undo_stack().push(
                a.command_set_transition(time, transition)
            );
        })
    ;
    register_from_meta<Reg, model::MetaAnimatable, model::AnimatableBase>(m);
}

class PyVisitorPublic : public model::Visitor
{
public:
    virtual void on_visit_doc(model::Document *){}
    virtual void on_visit_node(model::DocumentNode*){}

    void visit_nocomp(model::Document* doc, bool skip_locked)
    {
        visit(doc, nullptr, skip_locked);
    }

private:
    void on_visit_document(model::Document * document, model::Composition*) override
    {
        on_visit_doc(document);
    }

    void on_visit(model::DocumentNode * node) override
    {
        on_visit_node(node);
    }
};

class PyVisitorTrampoline : public PyVisitorPublic
{
public:
    void on_visit_doc(model::Document * document) override
    {
        PYBIND11_OVERLOAD(void, PyVisitorPublic, on_visit_doc, document);
    }

    void on_visit(model::DocumentNode * node) override
    {
        PYBIND11_OVERLOAD_PURE(void, PyVisitorPublic, on_visit_node, node);
    }
};


} // namespace


void register_py_module(py::module& glaxnimate_module)
{
    glaxnimate_module.attr("__version__") = AppInfo::instance().version();

    define_utils(glaxnimate_module);
    define_log(glaxnimate_module);
    py::module detail = define_detail(glaxnimate_module);
    define_environment(glaxnimate_module);

    // for some reason some classes aren't seen without this o_O
    static std::vector<int> foo = {
        qMetaTypeId<model::DocumentNode*>(),
        qMetaTypeId<model::NamedColor*>(),
        qMetaTypeId<model::Bitmap*>(),
        qMetaTypeId<model::Gradient*>(),
        qMetaTypeId<model::EmbeddedFont*>(),
        qMetaTypeId<io::ImportExport::Direction>(),
    };

    py::module model = glaxnimate_module.def_submodule("model", "");
    script::register_from_meta<Reg, model::Object, QObject>(model)
        .def(
            "stretch_time",
            [](model::Object* object, double multiplier){
                if ( multiplier > 0 )
                    object->push_command(new command::StretchTimeCommand(object, multiplier));
            },
            py::arg("multiplier"),
            "Stretches animation timings by the given factor"
        )
        .def_property_readonly("document", [](const model::Object* object) { return object->document(); })
        .def_property_readonly("grouped_animations", [](model::Object* object) { return &object->grouped_animations(); })
    ;

    py::class_<command::UndoMacroGuard>(model, "UndoMacroGuard")
        .def("__enter__", &command::UndoMacroGuard::start)
        .def("__exit__", [](command::UndoMacroGuard& g, pybind11::object, pybind11::object, pybind11::object){
            g.finish();
        })
        .def("start", &command::UndoMacroGuard::start)
        .def("finish", &command::UndoMacroGuard::finish)
        .attr("__doc__") = "Context manager that creates undo macros"
    ;

    register_from_meta<Reg, model::DocumentNode, model::Object>(model)
        .def_property_readonly("users", &model::DocumentNode::users, "List of properties pointing to this object")
    ;

    auto document = register_from_meta<Reg, model::Document, QObject>(model)
        .def(py::init<QString>())
        .def(py::init<>())
        .def(
            "macro",
             [](model::Document* document, const QString& str){
                return new command::UndoMacroGuard(str, document, false);
            },
            py::return_value_policy::take_ownership,
            "Context manager to group changes into a single undo command"
        )
        .def_property("metadata", &model::Document::metadata, &model::Document::set_metadata, no_own)
        .def_property("info", &model::Document::info, [](model::Document* doc, const model::Document::DocumentInfo& info){ doc->info() = info; }, no_own)
    ;
    py::class_<model::Document::DocumentInfo>(document, "DocumentInfo")
        .def_readwrite("description", &model::Document::DocumentInfo::description)
        .def_readwrite("author", &model::Document::DocumentInfo::author)
        .def_readwrite("keywords", &model::Document::DocumentInfo::keywords)
    ;

    register_top_level<Reg>(model);

    py::module shapes = model.def_submodule("shapes", "");
    register_from_meta<Reg, model::ShapeElement, model::VisualNode>(shapes)
        .def("to_path", &model::ShapeElement::to_path)
    ;

    py::module defs = model.def_submodule("assets", "");
    py::class_<model::AssetBase>(defs, "AssetBase");
    auto cls_comp = register_from_meta<Reg, model::Composition, model::VisualNode, model::AssetBase>(model);
    Reg::define_add_shape(cls_comp);

    define_io(glaxnimate_module);

    define_animatable(model);
    register_from_meta<Reg, model::detail::AnimatedPropertyPosition, model::AnimatedPropertyBase>(detail);
    register_animatable<QPointF, model::detail::AnimatedPropertyPosition>(detail);
    register_animatable<QSizeF>(detail);
    register_animatable<QVector2D>(detail);
    register_animatable<QColor>(detail);
    register_animatable<float>(detail);
    register_animatable<int>(detail);
    register_animatable<QGradientStops>(detail);
    register_from_meta<Reg, model::detail::AnimatedPropertyBezier, model::AnimatedPropertyBase>(detail);
    register_animatable<math::bezier::Bezier, model::detail::AnimatedPropertyBezier>(detail);

    py::class_<PyVisitorPublic, PyVisitorTrampoline>(model, "Visitor")
        .def(py::init())
        .def("visit", (void (PyVisitorPublic::*)(model::Document*, model::Composition*, bool))&PyVisitorPublic::visit, py::arg("document"), py::arg("composition"), py::arg("skip_locked"))
        .def("visit", (void (PyVisitorPublic::*)(model::Document*, bool))&PyVisitorPublic::visit_nocomp, py::arg("document"), py::arg("skip_locked"))
        .def("visit", (void (PyVisitorPublic::*)(model::DocumentNode*, bool))&PyVisitorPublic::visit, py::arg("node"), py::arg("skip_locked"))
        .def("on_visit_document", &PyVisitorPublic::on_visit_doc)
        .def("on_visit_node", &PyVisitorPublic::on_visit_node)
    ;

    register_from_meta<Reg, model::Asset, model::DocumentNode, model::AssetBase>(defs);
    register_assets<Reg>(defs);

    register_shapes<Reg>(shapes);

}
