#include "lottie_format.hpp"

#include <QJsonObject>
#include <QJsonArray>

using namespace model;

io::Autoreg<io::lottie::LottieFormat> io::lottie::LottieFormat::autoreg;


class LottieExporterState
{
public:
    struct FieldInfo
    {
        QString name;
        QString lottie;
        bool essential = false;
    };

    explicit LottieExporterState(model::Document* document)
        : document(document) {}

    QJsonObject to_json()
    {
        /// @todo make a system that preserves key order as that is needed for lottie android
        return convert_animation(document->animation());
    }

    QJsonObject convert_animation(Animation* animation)
    {
        layer_indices.clear();
        QJsonObject json = convert_object_basic(animation);
        json["v"] = "5.5.2";
        json["ip"] = 0;

        QJsonArray layers;
        for ( const auto& layer : animation->layers )
            layers.append(convert_layer(layer.get()));

        json["layers"] = layers;
        return json;
    }

    int layer_index(Layer* layer)
    {
        if ( !layer )
            return -1;
        if ( !layer_indices.contains(layer->uuid.get()) )
            layer_indices[layer->uuid.get()] = layer_indices.size();
        return layer_indices[layer->uuid.get()];
    }

    QJsonObject convert_layer(Layer* layer)
    {
        QVariantMap special;
        int parent_index = layer_index(layer->parent.get());
        if ( parent_index != -1 )
            special["parent"] = parent_index;
        QJsonObject json = convert_object_basic(layer);
        json["ty"] = layer_types[layer->type_name()];
        json["ind"] = layer_index(layer);
        json["ks"] = convert_transform(layer->transform.get());
        /// \todo add opacity to layer and set it to ks.o
        return json;
    }

    QJsonObject convert_transform(Transform* tf)
    {
        QJsonObject json = convert_object_basic(tf);
        QJsonObject o;
        o["a"] = 0;
        o["k"] = 100;
        json["o"] = o;
        return json;
    }

    QJsonValue value_from_variant(const QVariant& v)
    {
        if ( v.userType() == QMetaType::QPointF )
        {
            auto vv = v.toPointF();
            return QJsonArray{vv.x(), vv.y()};
        }
        else if ( v.userType() == QMetaType::QVector2D )
        {
            auto vv = v.value<QVector2D>() * 100;
            return QJsonArray{vv.x(), vv.y()};
        }
        return QJsonValue::fromVariant(v);
    }

    QJsonObject convert_object_basic(model::Object* obj)
    {
        QJsonObject json_obj;
        for ( const QMetaObject* mo = obj->metaObject(); mo; mo = mo->superClass() )
            convert_object_properties(obj, fields[model::Object::naked_type_name(mo->className())], json_obj);
        return json_obj;
    }

    void convert_object_properties(model::Object* obj, const QVector<FieldInfo>& fields, QJsonObject& json_obj)
    {
        for ( const auto& field : fields )
        {
            model::BaseProperty * prop = obj->get_property(field.name);
            if ( !prop )
            {
                qWarning() << field.name << "is not a property";
                continue;
            }

            if ( prop->traits().flags & PropertyTraits::Animated )
            {
                json_obj[field.lottie] = convert_animated(static_cast<AnimatableBase*>(prop));
            }
            else
            {
                json_obj[field.lottie] = value_from_variant(prop->value());
            }
        }
    }

    QJsonObject convert_animated(AnimatableBase* prop)
    {
        /// @todo for position fields also add spatial bezier handles
        QJsonObject jobj;
        if ( prop->animated() )
        {
            /// @todo
            jobj["a"] = 1;
        }
        else
        {
            jobj["a"] = 0;
            jobj["k"] = value_from_variant(prop->value());
        }
        return jobj;
    }

    model::Document* document;
    QMap<QUuid, int> layer_indices;

    // static mapping data
    const QMap<QString, QVector<FieldInfo>> fields = {
        {"DocumentNode", {
            FieldInfo{"name",           "nm", false},
            FieldInfo{"uuid",           "mn", false},
        }},
        {"AnimationContainer", {
            FieldInfo{"last_frame",     "op"},
            FieldInfo{"first_frame",    "ip"},
        }},
        {"Animation", {
            // version v
            FieldInfo{"fps",            "fr"},
            // * ip
            FieldInfo{"last_frame",     "op"},
            FieldInfo{"width",          "w"},
            FieldInfo{"height",         "h"},
            // ddd
            // assets
            // comps
            // fonts
            // chars
            // markers
            // motion_blur
        }},
        {"Layer", {
            // ddd
            // hd
            // * ty
            // * parent
            // stretch sr
            // transform ks
            // auto_orient ao
            FieldInfo{"start_time",     "st"},
            // blend_mode bm
            // matte_mode tt
            // * ind
            // css_class cl
            // layer_html_id ln
            // hasMasks
            // masksProperties
            // effects ef
        }},
        {"SolidColorLayer", {
            FieldInfo{"color",          "sc"},
            FieldInfo{"height",         "sh"},
            FieldInfo{"width",          "sw"},
        }},
        {"Transform", {
            FieldInfo{"anchor_point",   "a"},
            // px py pz
            FieldInfo{"position",       "p"},
            FieldInfo{"scale",          "s"},
            FieldInfo{"rotation",       "r"},
            // opacity o
            // skew sk
            // skew_axis sa
        }}
    };
    const QMap<QString, int> layer_types = {
        {"SolidColorLayer", 1},
        {"EmptyLayer", 3},
        {"ShapeLayer", 4}
    };
};

bool io::lottie::LottieFormat::on_save(QIODevice& file, const QString&,
                                         model::Document* document, const QVariantMap& setting_values)
{
    file.write(to_json(document).toJson(setting_values["pretty"].toBool() ? QJsonDocument::Indented : QJsonDocument::Compact));
    return true;
}

QJsonDocument io::lottie::LottieFormat::to_json(model::Document* document)
{
    LottieExporterState exp(document);
    return QJsonDocument(exp.to_json());
}
