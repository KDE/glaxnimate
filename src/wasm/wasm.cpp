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

#include "glaxnimate/script/glaxnimate_model.hpp"

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
    using namespace glaxnimate::script;
    using Reg = JsRegistrar;

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

    auto glaxnimate_module = Reg::module();
    auto model = Reg::submodule(glaxnimate_module, "model");
    auto defs = Reg::submodule(glaxnimate_module, "assets");
    auto shapes = Reg::submodule(glaxnimate_module, "shapes");

    register_from_meta<Reg, model::Document, QObject>(model);
    register_from_meta<Reg, model::Object, QObject>(model);
    register_from_meta<Reg, model::DocumentNode, model::Object>(model);
    register_top_level<Reg>(model);


    emscripten::class_<model::KeyframeBase>(m, "Keyframe")
        .property("time", &model::KeyframeBase::time)
        .def_property("value", &model::KeyframeBase::value)
        .property("transition", &model::KeyframeBase::transition)
        ;

    register_from_meta<Reg, model::AnimatableBase, QObject>(model)
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
    register_from_meta<Reg, model::detail::AnimatedPropertyPosition, model::AnimatableBase>(model);
    register_animatable<QPointF, model::detail::AnimatedPropertyPosition>();
    register_animatable<QSizeF>();
    register_animatable<QVector2D>();
    register_animatable<QColor>();
    register_animatable<float>();
    register_animatable<int>();
    register_animatable<QGradientStops>();
    register_from_meta<Reg, model::detail::AnimatedPropertyBezier, model::AnimatableBase>(model);
    register_animatable<math::bezier::Bezier, model::detail::AnimatedPropertyBezier>();

    auto cls_comp = register_from_meta<Reg, model::Composition, model::VisualNode>(defs);
    Reg::define_add_shape(cls_comp);

    register_from_meta<Reg, model::ShapeElement, model::VisualNode>(shapes)
        .function("to_path", &model::ShapeElement::to_path)
    ;

    register_assets<Reg>(defs);
    register_shapes<Reg>(shapes);
}
