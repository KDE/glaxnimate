/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_mime.hpp"

#include <set>

#include "import_state.hpp"
#include "model/shapes/shape.hpp"
#include "model/assets/assets.hpp"
#include "model/visitor.hpp"
#include "app/log/log.hpp"

using namespace glaxnimate;

io::Autoreg<io::glaxnimate::GlaxnimateMime> io::glaxnimate::GlaxnimateMime::autoreg;

namespace {

class GetDeps : public model::Visitor
{
public:
    GetDeps(const std::vector<model::DocumentNode*>& objects)
        : skip(objects.begin(), objects.end())
    {}

    void on_visit(model::DocumentNode * node) override
    {
        for ( auto property : node->properties() )
        {
            if ( property->traits().type == model::PropertyTraits::ObjectReference && property->name() != "parent" )
            {
                auto ptr = static_cast<model::ReferencePropertyBase*>(property)->get_ref();
                if ( !ptr || skip.count(ptr))
                    continue;

                skip.insert(ptr);
                referenced[ptr->uuid.get().toString()] = ptr;

                on_visit(ptr);
            }
        }
    }

    std::set<model::DocumentNode*> skip;
    std::map<QString, model::DocumentNode*> referenced;
};

} // namespace

QStringList io::glaxnimate::GlaxnimateMime::mime_types() const
{
    return {"application/vnd.glaxnimate.rawr+json"};
}

QJsonDocument io::glaxnimate::GlaxnimateMime::serialize_json(const std::vector<model::DocumentNode *>& objects)
{
    QJsonArray arr;
    GetDeps gd(objects);

    for ( auto object : objects )
    {
        gd.visit(object);
        arr.push_back(GlaxnimateFormat::to_json(object));
    }

    for ( const auto& p: gd.referenced )
        arr.push_front(GlaxnimateFormat::to_json(p.second));

    return QJsonDocument(arr);
}

QByteArray io::glaxnimate::GlaxnimateMime::serialize(const std::vector<model::DocumentNode*>& objects) const
{
    return serialize_json(objects).toJson(QJsonDocument::Compact);
}

io::mime::DeserializedData io::glaxnimate::GlaxnimateMime::deserialize(const QByteArray& data)  const
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(data);
    } catch ( const QJsonParseError& err ) {
        message(GlaxnimateFormat::tr("Could not parse JSON: %1").arg(err.errorString()));
        return {};
    }

    if ( !jdoc.isArray() )
    {
        message(GlaxnimateFormat::tr("No JSON object found"));
        return {};
    }

    QJsonArray input_objects = jdoc.array();

    io::mime::DeserializedData output;
    output.initialize_data();
    detail::ImportState state(nullptr, output.document.get());


    for ( auto json_val : input_objects )
    {
        if ( !json_val.isObject() )
            continue;

        QJsonObject json_object = json_val.toObject();
        auto obj = model::Factory::instance().build(json_object["__type__"].toString(), output.document.get());
        if ( !obj )
            continue;

        if ( auto shape = qobject_cast<model::ShapeElement*>(obj) )
        {
            output.main->shapes.emplace(shape);
        }
        else if ( auto composition = qobject_cast<model::Composition*>(obj) )
        {
            output.main->assign_from(composition);
            delete composition;
        }
        else if ( auto color = qobject_cast<model::NamedColor*>(obj) )
        {
            output.document->assets()->colors->values.emplace(color);
        }
        else if ( auto bitmap = qobject_cast<model::Bitmap*>(obj) )
        {
            output.document->assets()->images->values.emplace(bitmap);
        }
        else if ( auto gradient = qobject_cast<model::Gradient*>(obj) )
        {
            output.document->assets()->gradients->values.emplace(gradient);
        }
        else if ( auto gradient_colors = qobject_cast<model::GradientColors*>(obj) )
        {
            output.document->assets()->gradient_colors->values.emplace(gradient_colors);
        }
        else
        {
            app::log::Log("I/O").stream() << "Could not deserialize " << obj->type_name();
            delete obj;
            continue;
        }

        state.load_object(obj, json_object);
    }

    state.resolve();
    return output;
}
