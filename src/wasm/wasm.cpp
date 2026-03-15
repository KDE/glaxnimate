/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <emscripten/bind.h>

#include <QCoreApplication>
#include <QMetaProperty>

#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/renderer/renderer.hpp"
#include "glaxnimate/model/assets/assets.hpp"

#include "glaxnimate/model/shapes/style/stroke.hpp"
#include "glaxnimate/model/shapes/style/fill.hpp"

#include "glaxnimate/model/shapes/shapes/ellipse.hpp"
#include "glaxnimate/model/shapes/shapes/polystar.hpp"
#include "glaxnimate/model/shapes/shapes/rect.hpp"
#include "glaxnimate/model/shapes/shapes/path.hpp"

#include "glaxnimate/model/shapes/composable/layer.hpp"
#include "glaxnimate/model/shapes/composable/precomp_layer.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"

#include "glaxnimate/model/shapes/modifiers/repeater.hpp"
#include "glaxnimate/model/shapes/modifiers/trim.hpp"
#include "glaxnimate/model/shapes/modifiers/inflate_deflate.hpp"
#include "glaxnimate/model/shapes/modifiers/zig_zag.hpp"
#include "glaxnimate/model/shapes/modifiers/offset_path.hpp"
#include "glaxnimate/model/shapes/modifiers/round_corners.hpp"


#include "js_registrar.hpp"

using namespace glaxnimate;

namespace glaxnimate::js {
class MetaObject
{
public:
    MetaObject(const QMetaObject* meta) : meta(meta)
    {
        load_props();
    }

    emscripten::val properties() const
    {
        emscripten::val result = emscripten::val::array();
        for ( int i = 0; i < meta->propertyCount(); i++ )
            result.set(i, emscripten::val(meta->property(i).name()));
        return result;
    }

    QString class_name() const
    {
        return QString::fromLatin1(meta->className());
    }

    emscripten::val get(QObject* self, const std::string& name) const
    {
        auto it = props.find(name);
        if ( it == props.end() )
            return emscripten::val::undefined();
        return qvariant_to_val(it->second.read(self));
    }

    bool set(QObject* self, const std::string& name, const QVariant& value) const
    {
        auto it = props.find(name);
        if ( it == props.end() )
            return false;
        return it->second.write(self, value);
    }

    static MetaObject* from(const QObject* object)
    {
        if ( !object )
            return nullptr;
        return accessor(object->metaObject());
    }

    static MetaObject* accessor(const QMetaObject* obj)
    {
        static std::unordered_map<const QMetaObject*, MetaObject> metas;
        auto it = metas.find(obj);
        if ( it != metas.end() )
            return &it->second;

        return &metas.emplace(obj, obj).first->second;
    }

private:
    void load_props()
    {
        for ( int i = 0; i < meta->propertyCount(); i++ )
        {
            auto prop = meta->property(i);
            props[prop.name()] = std::move(prop);
        }
    }

    const QMetaObject* meta;
    std::unordered_map<std::string, QMetaProperty> props;
};

class GlaxnimateRenderer
{
public:
    GlaxnimateRenderer(const emscripten::val& options)
    {
        auto js_data = options["data"];
        if ( js_data.isUndefined() )
            return;

        QString filename = options["filename"].isUndefined() ? "data" : options["filename"].as<QString>();
        QByteArray data;
        if ( js_data.isString() )
            data = js_data.as<QString>().toUtf8();
        else
            data = js_data.as<QByteArray>();
        io::ImportExport* importer = nullptr;

        if ( !options["format"].isUndefined() && !options["format"].isNull() )
        {
            importer = io::IoRegistry::instance().from_slug(options["format"].as<QString>());
        }
        else if ( !options["filename"].isUndefined() )
        {
            importer = io::IoRegistry::instance().from_filename(options["filename"].as<QString>(), glaxnimate::io::ImportExport::Import);
        }

        if ( !importer || !importer->can_handle(glaxnimate::io::ImportExport::Import) )
            return;

        document = std::make_unique<model::Document>(filename);
        if ( !importer->load(document.get(), data, {}, filename) )
        {
            document.reset();
            return;
        }

        auto comps = document->assets()->compositions.get();
        if ( comps->values.empty() )
        {
            document.reset();
            return;
        }

        comp = comps->values[0];
    }

    model::FrameTime current_time() const
    {
        return document->current_time();
    }

    void set_current_time(model::FrameTime t)
    {
        document->set_current_time(t);
    }

    void on_render()
    {
        if ( !comp )
            return;
        frame = comp->render_image(current_time()).convertToFormat(QImage::Format_RGBA8888);
    }

    emscripten::val render()
    {
        on_render();
        const uchar* bits = frame.constBits();
        return emscripten::val(
            emscripten::typed_memory_view(frame.sizeInBytes(), bits)
        );
    }

    bool loaded() const
    {
        return comp;
    }

    float fps() const
    {
        return !comp ? 0 : comp->fps.get();
    }

    float width() const
    {
        return !comp ? 0 : comp->width.get();
    }

    float height() const
    {
        return !comp ? 0 : comp->height.get();
    }

    float last_frame() const
    {
        return !comp ? 0 : comp->animation->last_frame.get();
    }

    model::Document* document_obj() const
    {
        return (document.get());
    }

    model::Composition* composition() const
    {
        return comp;
    }

private:
    QImage frame;
    std::unique_ptr<model::Document> document;
    model::Composition* comp = nullptr;

};



template<class T, class Base=model::AnimatableBase>
void register_animatable()
{
    std::string name = "AnimatedProperty<";
    name += QMetaType::fromType<T>().name();
    name += ">";
    emscripten::class_<model::AnimatedProperty<T>, emscripten::base<Base>>(name.c_str());
}

void initialize()
{
    static int argc = 0;
    static char* argv[0] = {};
    static QCoreApplication app(argc, argv);
    io::IoRegistry::load_formats();
}

} // glaxnimate::js


EMSCRIPTEN_BINDINGS(glaxnimate_wasm)
{
    using namespace glaxnimate::js;
    emscripten::register_optional<emscripten::val>();
    emscripten::function("initialize", &initialize);

    emscripten::class_<MetaObject>("MetaObject")
        .class_function("from", &MetaObject::from, emscripten::allow_raw_pointers())
        .property("properties", &MetaObject::properties)
        .property("class_name", &MetaObject::class_name)
        .function("get", &MetaObject::get, emscripten::allow_raw_pointers())
    ;

    emscripten::class_<QObject>("QObject")
        .property("objectName", &QObject::objectName, qOverload<const QString&>.of<void, QObject>(&QObject::setObjectName))
        .function("meta", fn([](const QObject& obj){
            return MetaObject::accessor(obj.metaObject());
        }), emscripten::allow_raw_pointers());
    ;
    emscripten::value_object<QColor>("Color")
        .field("red", &QColor::red, &QColor::setRed)
        .field("green", &QColor::green, &QColor::setGreen)
        .field("blue", &QColor::blue, &QColor::setBlue)
        .field("alpha", &QColor::alpha, &QColor::setAlpha)
    ;
    emscripten::value_object<QPointF>("Point")
        .field("x", &QPointF::x, &QPointF::setX)
        .field("y", &QPointF::y, &QPointF::setY)
    ;
    emscripten::value_object<QVector2D>("Vector2D")
        .field("x", &QVector2D::x, &QVector2D::setX)
        .field("y", &QVector2D::y, &QVector2D::setY)
        .field("_v",
            fn([](const QVector2D&){ return true;}),
            fn([](QVector2D&, bool){})
        )
    ;
    emscripten::value_object<QSizeF>("Size")
        .field("width", &QSizeF::width, &QSizeF::setWidth)
        .field("height", &QSizeF::height, &QSizeF::setHeight)
    ;
    emscripten::class_<GlaxnimateRenderer>("GlaxnimateRenderer")
        .constructor<emscripten::val>()
        .property("current_time", &GlaxnimateRenderer::current_time, &GlaxnimateRenderer::set_current_time)
        .property("loaded", &GlaxnimateRenderer::loaded)
        .function("render", &GlaxnimateRenderer::render)
        .property("fps", &GlaxnimateRenderer::fps)
        .property("width", &GlaxnimateRenderer::width)
        .property("height", &GlaxnimateRenderer::height)
        .property("last_frame", &GlaxnimateRenderer::last_frame)
        .property("document", &GlaxnimateRenderer::document_obj, emscripten::return_value_policy::reference())
        .property("composition", &GlaxnimateRenderer::composition, emscripten::return_value_policy::reference())
    ;

    register_from_meta<model::Document, QObject>();

    register_from_meta<model::Object, QObject>();
    register_from_meta<model::DocumentNode, model::Object>();
    register_from_meta<model::VisualNode, model::DocumentNode>();

    register_from_meta<model::AnimationContainer, model::Object>();
    register_from_meta<model::StretchableTime, model::Object>();
    register_from_meta<model::Transform, model::Object>();
    register_from_meta<model::MaskSettings, model::Object>();

    register_from_meta<model::AnimatableBase, QObject>()
        .function("get", fn([](const model::AnimatableBase& anim){
            return anim.value();
        }))
        .function("get_at", fn([](const model::AnimatableBase& anim, double t){
            return anim.value(t);
        }))
        .function("set", fn([](model::AnimatableBase& anim, const QVariant& var){
            return anim.set_undoable(var);
        }))
    ;
    register_from_meta<model::detail::AnimatedPropertyPosition, model::AnimatableBase>();
    register_animatable<QPointF, model::detail::AnimatedPropertyPosition>();
    register_animatable<QSizeF>();
    register_animatable<QVector2D>();
    register_animatable<QColor>();
    register_animatable<float>();
    register_animatable<int>();
    register_animatable<QGradientStops>();
    register_from_meta<model::detail::AnimatedPropertyBezier, model::AnimatableBase>();
    register_animatable<math::bezier::Bezier, model::detail::AnimatedPropertyBezier>();

    register_from_meta<model::Assets, model::DocumentNode>();
    register_from_meta<model::Asset, model::DocumentNode>();
    register_from_meta<model::Composition, model::VisualNode>();
    register_from_meta<model::CompositionList, model::DocumentNode>();
    register_from_meta<model::NamedColor, model::Asset>();
    register_from_meta<model::NamedColorList, model::DocumentNode>();
    register_from_meta<model::GradientColors, model::Asset>();
    register_from_meta<model::GradientColorsList, model::DocumentNode>();
    register_from_meta<model::Gradient, model::Asset>();
    register_from_meta<model::GradientList, model::DocumentNode>();
    register_from_meta<model::Bitmap, model::Asset>();
    register_from_meta<model::BitmapList, model::DocumentNode>();
    register_from_meta<model::EmbeddedFont, model::Asset>();
    register_from_meta<model::FontList, model::DocumentNode>();

    register_from_meta<model::ShapeElement, model::VisualNode>();
    register_from_meta<model::Shape, model::ShapeElement>();
    register_from_meta<model::Modifier, model::ShapeElement>();
    register_from_meta<model::Styler, model::ShapeElement>();
    register_from_meta<model::Composable, model::ShapeElement>();

    register_constructible<model::Rect, model::Shape>();
    register_constructible<model::Ellipse, model::Shape>();
    register_constructible<model::PolyStar, model::Shape>(enums<model::PolyStar::StarType>{});
    register_constructible<model::Path, model::Shape>();

    auto cls_group = register_constructible<model::Group, model::Composable>();
    // define_add_shape(cls_group);

    register_constructible<model::Layer, model::Group>();
    register_constructible<model::PreCompLayer, model::Composable>();
    register_constructible<model::Image, model::Composable>();

    register_constructible<model::Fill, model::Styler>(enums<model::Fill::Rule>{});
    register_constructible<model::Stroke, model::Styler>(enums<model::Stroke::Cap, model::Stroke::Join>{});
    register_constructible<model::Repeater, model::Modifier>();

    register_from_meta<model::PathModifier, model::Modifier>();
    register_constructible<model::Trim, model::PathModifier>();
    register_constructible<model::InflateDeflate, model::PathModifier>();
    register_constructible<model::RoundCorners, model::PathModifier>();
    register_constructible<model::OffsetPath, model::PathModifier>();
    register_constructible<model::ZigZag, model::PathModifier>();
}
