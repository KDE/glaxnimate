/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <emscripten/bind.h>

#include <QMetaProperty>

#include "glaxnimate/io/io_registry.hpp"
#include "glaxnimate/renderer/renderer.hpp"
#include "glaxnimate/model/assets/assets.hpp"

using namespace glaxnimate;

namespace glaxnimate::js {

emscripten::val convert_qvariant(const QVariant& v);

class MetaObjectAccessor
{
private:
public:
    MetaObjectAccessor(const QMetaObject* meta)
    : meta(meta)
    {
        load_props();
    }

    emscripten::val get(QObject* self, const std::string& name) const
    {
        auto it = props.find(name);
        if ( it == props.end() )
            return emscripten::val::undefined();
        return convert_qvariant(it->second.read(self));
    }

    static MetaObjectAccessor* accessor(const QMetaObject* obj)
    {
        static std::unordered_map<const QMetaObject*, MetaObjectAccessor> metas;
        auto it = metas.find(obj);
        if ( it != metas.end() )
            return &it->second;

        return &metas.emplace(obj, obj).first->second;
    }

// private:
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

class ObjectAccessor
{
public:
    ObjectAccessor(QObject* object)
        : object(object),
        accessor(MetaObjectAccessor::accessor(object->metaObject()))
    {}

    std::string class_name() const
    {
        return accessor->meta->className();
    }

    emscripten::val get(const std::string& name)
    {
        return accessor->get(object, name);
    }

    std::string to_string() const
    {
        return object->objectName().isEmpty() ? class_name() : object->objectName().toStdString();
    }

    emscripten::val properties() const
    {
        /*std::vector<emscripten::val> properties;
        properties.reserve(accessor->props.size());
        for ( const auto& p : accessor->props )
            properties.push_back(emscripten::val(p.first));
        return emscripten::val::array(properties.begin(), properties.end());*/

        emscripten::val properties = emscripten::val::array();
        properties.call<void>("push", emscripten::val("hello"));
        for ( const auto& p : accessor->props )
            properties.call<void>("push", emscripten::val(p.first));
        return properties;
    }

private:
    QObject* object;
    MetaObjectAccessor* accessor;
};


emscripten::val convert_qvariant(const QVariant& v)
{
    switch ( v.typeId() )
    {
        case QMetaType::Bool:
            return emscripten::val(v.toBool());
        case QMetaType::Float:
            return emscripten::val(v.toFloat());
        case QMetaType::Double:
            return emscripten::val(v.toDouble());
        case QMetaType::Long:
        case QMetaType::Short:
        case QMetaType::Int:
            return emscripten::val(v.toInt());
        case QMetaType::LongLong:
            return emscripten::val(v.toLongLong());
        case QMetaType::ULong:
        case QMetaType::UShort:
        case QMetaType::UInt:
            return emscripten::val(v.toUInt());
        case QMetaType::ULongLong:
            return emscripten::val(v.toULongLong());
        case QMetaType::QString:
            return emscripten::val(v.toString().toStdString());
    }

    auto meta_obj = v.metaType().metaObject();
    if ( meta_obj )
    {
        return emscripten::val(ObjectAccessor(qvariant_cast<QObject*>(v)));
    }
    return emscripten::val::undefined();
}

class GlaxnimateRenderer
{
public:
    GlaxnimateRenderer(const emscripten::val& options)
    {
        static bool initialized = false;
        if ( !initialized )
        {
            io::IoRegistry::load_formats();
            initialized = true;
        }

        if ( options["data"].isUndefined() )
            return;

        QString filename = options["filename"].isUndefined() ? "data" : QString::fromStdString(options["filename"].as<std::string>());
        QByteArray data = QByteArray::fromStdString(options["data"].as<std::string>());
        io::ImportExport* importer = nullptr;

        if ( !options["slug"].isUndefined() )
        {
            importer = io::IoRegistry::instance().from_slug(QString::fromStdString(options["slug"].as<std::string>()));
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

    ObjectAccessor document_obj() const
    {
        return ObjectAccessor(document.get());
    }

private:
    QImage frame;
    std::unique_ptr<model::Document> document;
    model::Composition* comp = nullptr;

};

} // glaxnimate::js


EMSCRIPTEN_BINDINGS(my_module)
{
    using namespace glaxnimate::js;
    emscripten::function("initialize", &io::IoRegistry::load_formats);
    emscripten::class_<QObject>("QObject");
    emscripten::class_<GlaxnimateRenderer>("GlaxnimateRenderer")
        .constructor<emscripten::val>()
        .property("current_time", &GlaxnimateRenderer::current_time, &GlaxnimateRenderer::set_current_time)
        .property("loaded", &GlaxnimateRenderer::loaded)
        .property("fps", &GlaxnimateRenderer::fps)
        .property("width", &GlaxnimateRenderer::width)
        .property("height", &GlaxnimateRenderer::height)
        .property("last_frame", &GlaxnimateRenderer::last_frame)
        .function("render", &GlaxnimateRenderer::render)
        .property("document", &GlaxnimateRenderer::document_obj, emscripten::return_value_policy::reference())
    ;
    emscripten::class_<ObjectAccessor>("ObjectAccessor")
        .constructor<QObject*>()
        .property("class_name", &ObjectAccessor::class_name)
        .function("toString", &ObjectAccessor::to_string)
        .function("get", &ObjectAccessor::get)
        .function("properties", &ObjectAccessor::properties)
    ;
}
