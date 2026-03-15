/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <QCborValue>
#include <QCborArray>

#include "glaxnimate/io/lottie/cbor_write_json.hpp"
#include "glaxnimate/io/lottie/lottie_private_common.hpp"
#include "glaxnimate/model/animation/join_animatables.hpp"
#include "glaxnimate/app_info.hpp"
#include "glaxnimate/io/utils.hpp"

namespace glaxnimate::io::lottie::detail {

inline QLatin1String operator ""_l(const char* c, std::size_t sz)
{
    return QLatin1String(c, sz);
}

// inline QString debug_value(QCborValue v)
// {
//     QCborMap map;
//     map["v"_l] = v;
//     return QString(cbor_write_json(map, false));
// }

class LottieExporterState
{
    static constexpr const char* version = "5.13.0";

public:
    explicit LottieExporterState(ImportExport* format, model::Composition* comp, bool strip, bool strip_raster, const QVariantMap& settings )
        : format(format),
        main(comp),
        document(comp->document()),
        strip(strip),
        strip_raster( strip_raster ),
        auto_embed(settings["auto_embed"].toBool())
    {}

    QCborMap to_json()
    {
        return convert_main(main);
    }

    void convert_animation_container(model::AnimationContainer* animation, QCborMap& json)
    {
        json["ip"_l] = animation->first_frame.get();
        json["op"_l] = animation->last_frame.get();
    }

    void convert_composition(model::Composition* composition, QCborMap& json)
    {
        QCborArray layers;
        for ( const auto& layer : composition->shapes )
        {
            if ( !strip || layer->visible.get() )
            {
                wrap_to_layer.clear();
                shape_is_layer(layer.get());
                convert_as_layer(layer.get(), layers, nullptr, true, {}, composition->animation.get());
            }
        }

        json["layers"_l] = layers;
    }

    QCborMap convert_main(model::Composition* animation)
    {
        layer_indices.clear();
        QCborMap json;
        json["v"_l] = version;
        convert_animation_container(animation->animation.get(), json);
        convert_object_basic(animation, json);
        json["assets"_l] = convert_assets(animation);
        convert_composition(animation, json);
        if ( !strip )
            convert_meta(json);
        return json;
    }

    void convert_meta(QCborMap& json)
    {
        QCborMap meta;
        meta["g"_l] = QString("%1 %2").arg(AppInfo::instance().name(), AppInfo::instance().version());

        if ( !document->info().description.isEmpty() )
            meta["d"_l] = document->info().description;

        if ( !document->info().author.isEmpty() )
            meta["a"_l] = document->info().author;

        if ( !document->info().keywords.isEmpty() )
        {
            QCborArray k;
            for ( const auto& kw : document->info().keywords )
                k.push_back(kw);
            meta["k"_l] = k;
        }

        json["meta"_l] = meta;
    }

    int layer_index(model::DocumentNode* layer)
    {
        if ( !layer )
            return -1;
        if ( !layer_indices.contains(layer->uuid.get()) )
            layer_indices[layer->uuid.get()] = layer_indices.size();
        return layer_indices[layer->uuid.get()];
    }

    struct MatteData
    {
        int type;
        int parent;
        MatteData() : type(-1), parent(-1) {};

        bool has_matte() const { return parent != -1 && type != -1; }
    };

    QCborMap convert_as_layer(
        model::ShapeElement* shape,
        QCborArray& output,
        model::ShapeElement* parent,
        bool push,
        const MatteData& matte,
        model::AnimationContainer* animation
    )
    {
        QCborMap json;
        json["ddd"_l] = 0;
        int index = layer_index(shape);
        json["ind"_l] = index;
        json["st"_l] = 0;
        json["ty"_l] = 3;
        if ( !shape->visible.get() )
        {
            if ( strip )
                return {};
            json["hd"_l] = true;
        }

        if ( !strip )
        {
            json["nm"_l] = shape->name.get();
            json["uid"_l] = shape->uuid.get().toString();
        }

        if ( auto layer = shape->cast<model::Layer>() )
        {
            if ( !layer->render.get() )
                return {};
            convert_normal_layer(layer, json, output);
        }
        else if ( auto image = shape->cast<model::Image>() )
        {
            convert_image_layer(image, json, animation);
        }
        else if ( auto precomp = shape->cast<model::PreCompLayer>() )
        {
            convert_precomp_layer(precomp, json, animation);
        }
        else if ( auto group = shape->cast<model::Group>() )
        {
            group_as_layer(group, json, animation, output);
        }
        else
        {
            shape_as_layer(shape, json, animation);
        }

        if ( parent )
            json["parent"_l] = layer_index(parent);

        if ( matte.has_matte() )
        {
            json["tt"_l] = matte.type;
            json["tp"_l] = matte.parent;
        }

        if ( push )
            output.push_front(json);
        return json;
    }

    void convert_normal_layer(model::Layer* layer, QCborMap& json, QCborArray& output)
    {
        auto animation = layer->animation.get();
        convert_animation_container(animation, json);
        convert_object_properties(layer, fields["__Layer__"], json);
        convert_composable(layer, json);

        if ( !layer->shapes.empty() )
        {
            bool all_shapes = !contains_layer.count(layer);

            if ( all_shapes && !layer->mask->has_mask() )
            {
                json["ty"_l] = 4;
                json["shapes"_l] = convert_shapes(layer->shapes, false);
            }
            else
            {
                int i = 0;
                MatteData matte;
                if ( layer->mask->has_mask() && !layer->shapes.empty() )
                {
                    if ( layer->shapes[0]->visible.get() )
                    {
                        QCborMap mask = convert_as_layer(layer->shapes[0], output, layer, false, {}, animation);
                        if ( !mask.isEmpty() )
                        {
                            mask["td"_l] = 1;
                            // mask["hd"_l] = 1;
                            output.push_front(mask);
                            matte.parent = mask["ind"_l].toInteger();
                            matte.type = convert_matte_type(layer->mask.get());
                        }
                    }
                    i = 1;
                }

                for ( ; i < layer->shapes.size(); i++ )
                {
                    convert_as_layer(layer->shapes[i], output, layer, true, matte, animation);
                }
            }
        }
    }

    void group_as_layer(model::Group* group, QCborMap& json, model::AnimationContainer* animation, QCborArray& output)
    {
        if ( contains_layer.count(group) )
        {
            convert_composable(group, json);
            for ( const auto& child : group->shapes )
                convert_as_layer(child.get(), output, group, true, {}, animation);
        }
        else
        {
            shape_as_layer(group, json, animation);
        }
    }

    void shape_as_layer(model::ShapeElement* shape, QCborMap& json, model::AnimationContainer* animation)
    {
        convert_animation_container(animation, json);
        json["ty"_l] = 4;

        if ( auto grp = shape->cast<model::Group>() )
        {
            convert_composable(grp, json);
            json["shapes"_l] = convert_shapes(grp->shapes, false);
        }
        else
        {
            json["ks"_l] = QCborMap();

            QCborArray shapes;
            shapes.push_back(convert_shape(shape, false));
            json["shapes"_l] = shapes;
        }
    }

    enum class LayerType { Shape, Layer, Image, PreComp };

    LayerType layer_type(model::ShapeElement* shape)
    {
        auto meta = shape->metaObject();
        if ( meta->inherits(&model::Layer::staticMetaObject) )
            return LayerType::Layer;
        if ( meta->inherits(&model::Image::staticMetaObject) )
            return LayerType::Image;
        if ( meta->inherits(&model::PreCompLayer::staticMetaObject) )
            return LayerType::PreComp;
        return LayerType::Shape;
    }

    void convert_composable(model::Composable* grp, QCborMap& json)
    {
        convert_object_properties(grp, fields["Composable"], json);
        QCborMap transform;
        convert_transform(grp->transform.get(), &grp->opacity, transform);
        json["ks"_l] = transform;
        if ( grp->transform->auto_orient.get() )
            json["ao"_l] = 1;
        if ( grp->blend_mode.get() != renderer::BlendMode::Normal )
            json["bm"_l] = int(grp->blend_mode.get());
    }


    int convert_matte_type(model::MaskSettings* mask)
    {
        switch ( mask->mask.get() )
        {
            case model::MaskSettings::NoMask:
                return 0;
            case model::MaskSettings::Alpha:
                return mask->inverted.get() ? 2 : 1;
            case model::MaskSettings::Luma:
                return mask->inverted.get() ? 4 : 3;
        }
        return -1;
    }

    void convert_transform(model::Transform* tf, model::AnimatableBase* opacity, QCborMap& json)
    {
        convert_object_basic(tf, json);
        if ( opacity )
            json["o"_l] = convert_animated(opacity, FloatMult(100));
        else
            json["o"_l] = fake_animated(100);
    }

    QCborArray point_to_lottie(const QPointF& vv)
    {
        return QCborArray{vv.x(), vv.y()};
    }

    QCborValue value_from_variant(const QVariant& v)
    {
        switch ( v.userType() )
        {
            case QMetaType::QPointF:
                return point_to_lottie(v.toPointF());
            case QMetaType::QVector2D:
            {
                auto vv = v.value<QVector2D>() * 100;
                return QCborArray{vv.x(), vv.y()};
            }
            case QMetaType::QSizeF:
            {
                auto vv = v.toSizeF();
                return QCborArray{vv.width(), vv.height()};
            }
            case QMetaType::QColor:
            {
                auto vv = v.value<QColor>().toRgb();
                return QCborArray{vv.redF(), vv.greenF(), vv.blueF()};
            }
            case QMetaType::QUuid:
                return v.toString();
        }

        if ( v.userType() == qMetaTypeId<math::bezier::Bezier>() )
        {
            math::bezier::Bezier bezier = v.value<math::bezier::Bezier>();
            QCborMap jsbez;
            jsbez["c"_l] = bezier.closed();
            QCborArray pos, tan_in, tan_out;
            for ( const auto& p : bezier )
            {
                pos.push_back(point_to_lottie(p.pos));
                tan_in.push_back(point_to_lottie(p.tan_in - p.pos));
                tan_out.push_back(point_to_lottie(p.tan_out - p.pos));
            }
            jsbez["v"_l] = pos;
            jsbez["i"_l] = tan_in;
            jsbez["o"_l] = tan_out;
            return jsbez;
        }
        else if ( v.userType() == qMetaTypeId<math::bezier::Point>() )
        {
            return point_to_lottie(v.value<math::bezier::Point>().pos);
        }
        else if ( v.userType() == qMetaTypeId<QGradientStops>() )
        {
            QCborArray weird_ass_representation;
            auto gradient = v.value<QGradientStops>();
            bool alpha = false;
            for ( const auto& stop : gradient )
            {
                weird_ass_representation.push_back(stop.first);
                weird_ass_representation.push_back(stop.second.redF());
                weird_ass_representation.push_back(stop.second.greenF());
                weird_ass_representation.push_back(stop.second.blueF());
                alpha = alpha || stop.second.alpha() != 0;
            }
            if ( alpha )
            {
                for ( const auto& stop : gradient )
                {
                    weird_ass_representation.push_back(stop.first);
                    weird_ass_representation.push_back(stop.second.alphaF());
                }
            }
            return weird_ass_representation;
        }
        else if ( v.userType() >= QMetaType::User && v.canConvert<int>() )
        {
            return v.toInt();
        }
        return QCborValue::fromVariant(v);
    }

    void convert_object_from_meta(model::Object* obj, const QMetaObject* mo, QCborMap& json_obj)
    {
        if ( auto super = mo->superClass() )
            convert_object_from_meta(obj, super, json_obj);

        auto it = fields.find(model::detail::naked_type_name(mo));
        if ( it != fields.end() )
            convert_object_properties(obj, *it, json_obj);
    }

    void convert_object_basic(model::Object* obj, QCborMap& json_obj)
    {
        convert_object_from_meta(obj, obj->metaObject(), json_obj);
    }

    void convert_object_properties(model::Object* obj, const QVector<FieldInfo>& fields, QCborMap& json_obj)
    {
        for ( const auto& field : fields )
        {
            if ( field.mode != Auto || (strip && !field.essential) )
                continue;

            model::BaseProperty * prop = obj->get_property(field.name);
            if ( !prop )
            {
                logger.stream() << field.name << "is not a property";
                continue;
            }

            if ( prop->traits().flags & model::PropertyTraits::Animated )
            {
                json_obj[field.lottie] = convert_animated(static_cast<model::AnimatableBase*>(prop), field.transform);
            }
            else
            {
                json_obj[field.lottie] = value_from_variant(field.transform.to_lottie(prop->value(), 0));
            }
        }
    }

    QCborValue keyframe_value_from_variant(const QVariant& v)
    {
        auto cb = value_from_variant(v);
        if ( cb.isArray() )
            return cb;

        return QCborArray{cb};
    }

    void populate_keyframe(QCborMap& jkf, const QVariant& value, model::FrameTime time, const model::KeyframeTransition& transition, bool is_last)
    {
        QCborValue kf_value = keyframe_value_from_variant(value);

        jkf["t"_l] = time;
        jkf["s"_l] = kf_value;

        if ( !is_last )
        {
            if ( transition.hold() )
            {
                jkf["h"_l] =  1;
            }
            else
            {
                jkf["h"_l] =  0;
                jkf["o"_l] = keyframe_bezier_handle(transition.before());
                jkf["i"_l] = keyframe_bezier_handle(transition.after());
            }
        }
    }

    QCborMap convert_animated(
        model::AnimatableBase* prop,
        const TransformFunc& transform_values
    )
    {
        bool position = prop->traits().type == model::PropertyTraits::Point;

        QCborMap jobj;
        if ( prop->keyframe_count() > 1 )
        {
            jobj["a"_l] = 1;
            std::vector<std::unique_ptr<model::KeyframeBase>> split_kfs = split_keyframes(prop);

            QCborArray keyframes;
            QCborMap jkf;
            for ( int i = 0, e = split_kfs.size(); i < e; i++ )
            {
                auto kf = split_kfs[i].get();
                QVariant v = transform_values.to_lottie(kf->value(), kf->time());

                if ( i != 0 )
                {

                    if ( position )
                    {
                        auto pkf = static_cast<model::Keyframe<QPointF>*>(kf);
                        jkf["ti"_l] = point_to_lottie(pkf->point().tan_in - pkf->get());
                    }

                    keyframes.push_back(jkf);
                }

                jkf.clear();
                populate_keyframe(jkf, v, kf->time(), kf->transition(), i == e - 1);

                if ( position )
                {
                    auto pkf = static_cast<model::Keyframe<QPointF>*>(kf);
                    jkf["to"_l] = point_to_lottie(pkf->point().tan_out - pkf->get());
                }
            }
            if ( position )
                jkf.remove("to"_l);
            keyframes.push_back(jkf);
            jobj["k"_l] = keyframes;
        }
        else
        {
            jobj["a"_l] = 0;
            QVariant v = transform_values.to_lottie(prop->value(), 0);
            jobj["k"_l] = value_from_variant(v);
        }
        return jobj;
    }

    QCborMap keyframe_bezier_handle(const QPointF& p)
    {
        QCborMap jobj;
        QCborArray x;
        x.push_back(p.x());
        QCborArray y;
        y.push_back(p.y());
        jobj["x"_l] = x;
        jobj["y"_l] = y;
        return jobj;
    }

    static std::pair<qreal, qreal> radial_highlight(const QPointF& s, const QPointF& e, const QPointF& h)
    {
        auto de = e - s;
        auto dh = h - s;
        auto dist = math::hypot(de.x(), de.y());
        qreal val_h = qFuzzyIsNull(dist) ? 0 : math::hypot(dh.x(), dh.y()) / dist * 100;
        qreal val_a = math::rad2deg(math::atan2(dh.y(), dh.x()) - math::atan2(de.y(), de.x()));
        return {val_h, val_a};
    }

    void convert_styler(model::Styler* shape, QCborMap& jsh)
    {
        auto used = shape->use.get();

        auto gradient = qobject_cast<model::Gradient*>(used);
        if ( !gradient || !gradient->colors.get() )
        {
            auto color_prop = &shape->color;
            if ( auto color = qobject_cast<model::NamedColor*>(used) )
                color_prop = &color->color;
            jsh["c"_l] = convert_animated(color_prop, {});

            auto join_func = [](const std::vector<QVariant>& args) -> QVariant {
                return args[0].value<QColor>().alphaF() * args[1].toFloat() * 100;
            };
            model::JoinedAnimatable join({color_prop, &shape->opacity}, join_func);
            jsh["o"_l] = convert_animated(&join, {});
            return;
        }

        convert_object_basic(gradient, jsh);

        if ( shape->type_name() == "Fill" )
            jsh["ty"_l] = "gf";
        else
            jsh["ty"_l] = "gs";

        if ( gradient->type.get() == model::Gradient::Radial )
        {
            model::JoinAnimatables highlight({&gradient->start_point, &gradient->end_point, &gradient->highlight});
            QCborMap prop_a;
            QCborMap prop_h;
            if ( highlight.animated() )
            {
                prop_a["a"_l] = 1;
                prop_h["a"_l] = 1;
                QCborArray k_a;
                QCborArray k_h;
                for ( std::size_t i = 0; i < highlight.keyframes().size(); i++ )
                {
                    const auto& kf = highlight.keyframes()[i];
                    QCborMap kf_a;
                    QCborMap kf_h;
                    qreal val_h, val_a;
                    std::tie(val_h, val_a) = radial_highlight(kf.values[0].toPointF(), kf.values[1].toPointF(), kf.values[2].toPointF());
                    populate_keyframe(kf_h, val_h, kf.time, kf.transition(), i + 1 == highlight.keyframes().size());
                    populate_keyframe(kf_a, val_a, kf.time, kf.transition(), i + 1 == highlight.keyframes().size());
                    k_a.push_back(kf_a);
                    k_h.push_back(kf_h);
                }

                prop_a["k"_l] = k_a;
                prop_h["k"_l] = k_h;
            }
            else
            {
                prop_a["a"_l] = 0;
                prop_h["a"_l] = 0;
                qreal val_h, val_a;
                std::tie(val_h, val_a) = radial_highlight(gradient->start_point.get(), gradient->end_point.get(), gradient->highlight.get());
                prop_a["k"_l] = val_a;
                prop_h["k"_l] = val_h;
            }

            jsh["a"_l] = prop_a;
            jsh["h"_l] = prop_h;
        }

        auto colors = gradient->colors.get();
        QCborMap jcolors;
        jcolors["p"_l] = colors->colors.get().size();
        jcolors["k"_l] = convert_animated(&colors->colors, {});
        jsh["g"_l] = jcolors;
    }

    QCborMap convert_shape(model::ShapeElement* shape, bool force_hidden)
    {
        if ( auto text = shape->cast<model::TextShape>() )
        {
            auto conv = text->to_path();
            return convert_shape(conv.get(), force_hidden || !shape->visible.get());
        }

        QCborMap jsh;
        jsh["ty"_l] = shape_types[shape->type_name()];
//         jsh["d"] = 0;
        if ( force_hidden || !shape->visible.get() )
            jsh["hd"_l] = true;

        convert_object_basic(shape, jsh);

        if ( auto gr = qobject_cast<model::Group*>(shape) )
        {
            auto shapes = convert_shapes(gr->shapes, force_hidden || !gr->visible.get());
            QCborMap transform;
            transform["ty"_l] = "tr";
            convert_transform(gr->transform.get(), &gr->opacity, transform);
            shapes.push_back(transform);
            jsh["it"_l] = shapes;
        }
        else if ( auto styler = shape->cast<model::Styler>() )
        {
            convert_styler(styler, jsh);
        }
        else if ( auto polystar = shape->cast<model::PolyStar>() )
        {
            if ( polystar->type.get() == model::PolyStar::Polygon )
            {
                jsh.remove("is"_l);
                jsh.remove("ir"_l);
            }
        }
        else if ( auto styler = shape->cast<model::Repeater>() )
        {
            QCborMap transform;
            convert_transform(styler->transform.get(), nullptr, transform);
            transform.remove("o"_l);
            transform["so"_l] = convert_animated(&styler->start_opacity, FloatMult(100));
            transform["eo"_l] = convert_animated(&styler->end_opacity, FloatMult(100));
            jsh["o"_l] = fake_animated(0);
            jsh["m"_l] = 1;
            jsh["tr"_l] = transform;
        }
        else if ( !shape->is_instance<model::Shape>() )
        {
            format->warning(i18n("%1 is an unsupported shape of type %2", shape->object_name(), shape->type_name_human()));
        }

        return jsh;
    }

    QCborMap fake_animated(const QCborValue& val)
    {
        QCborMap fake;
        fake["a"_l] = 0;
        fake["k"_l] = val;
        return fake;
    }

    QCborArray convert_shapes(const model::ShapeListProperty& shapes, bool force_hidden)
    {
        QCborArray jshapes;
        for ( const auto& shape : shapes )
        {
            if ( shape->is_instance<model::Image>() )
                format->warning(i18n("Images cannot be grouped with other shapes, they must be inside a layer"));
            else if ( shape->is_instance<model::PreCompLayer>() )
                format->warning(i18n("Composition layers cannot be grouped with other shapes, they must be inside a layer"));
            else if ( !strip || shape->visible.get() )
                jshapes.push_front(convert_shape(shape.get(), force_hidden));
        }
        return jshapes;
    }

    QCborArray convert_assets(model::Composition* animation)
    {
        QCborArray assets;

        if ( !strip_raster )
        {
            for ( const auto& bmp : document->assets()->images->values )
            {
                if ( auto_embed && !bmp->embedded() )
                {
                    auto clone = bmp->clone_covariant();
                    clone->embed(true);
                    assets.push_back(convert_bitmat(clone.get()));
                }
                else
                {
                    assets.push_back(convert_bitmat(bmp.get()));
                }
            }
        }

        for ( const auto& comp : document->assets()->compositions->values )
        {
            if ( comp.get() != animation )
                assets.push_back(convert_precomp(comp.get()));
        }

        return assets;
    }

    QCborMap convert_bitmat(model::Bitmap* bmp)
    {
        QCborMap out;
        convert_object_basic(bmp, out);
        out["id"_l] = bmp->uuid.get().toString();
        out["e"_l] = int(bmp->embedded());
        if ( bmp->embedded() )
        {
            out["u"_l] = "";
            out["p"_l] = bmp->to_url().toString();
        }
        else
        {
            auto finfo = bmp->file_info();
            out["u"_l] = finfo.absolutePath();
            out["p"_l] = finfo.fileName();
        }
        return out;
    }

    void convert_image_layer(model::Image* image, QCborMap& json, model::AnimationContainer* animation)
    {
        convert_animation_container(animation, json);
        if ( !strip_raster )
            json["ty"_l] = 2;
        convert_composable(image, json);
        if ( !strip_raster && image->image.get() )
            json["refId"_l] = image->image->uuid.get().toString();
    }

    QCborMap convert_precomp(model::Composition* comp)
    {
        QCborMap out;
        convert_object_basic(comp, out);
        out["id"_l] = comp->uuid.get().toString();
        convert_composition(comp, out);
        return out;
    }

    void convert_precomp_layer(model::PreCompLayer* layer, QCborMap& json, model::AnimationContainer* animation)
    {
        json["ty"_l] = 0;
        json["ind"_l] = layer_index(layer);
        convert_animation_container(animation, json);
        json["st"_l] = layer->timing->start_time.get();
        json["sr"_l] = layer->timing->stretch.get();
        convert_composable(layer, json);
        if ( layer->composition.get() )
            json["refId"_l] = layer->composition->uuid.get().toString();
        json["w"_l] = layer->size.get().width();
        json["h"_l] = layer->size.get().height();
    }

    bool group_is_layer(model::Group* shape)
    {
        bool force_layer = false;
        if ( shape->transform->auto_orient.get() )
            force_layer = true;
        if ( shape->cast<model::Layer>() )
            force_layer = true;

        bool child_layer = false;
        for ( const auto& child : shape->shapes )
            if ( shape_is_layer(child.get()) )
                child_layer = true;

        if ( child_layer )
        {
            force_layer = true;
            contains_layer.insert(shape);
        }

        if ( force_layer )
            wrap_to_layer.insert(shape);
        return force_layer;
    }

    bool shape_is_layer(model::ShapeElement* shape)
    {
        if ( shape->cast<model::Composable>() )
        {
            if ( auto g = shape->cast<model::Group>() )
                return group_is_layer(g);
            return true;
        }

        return false;
    }

    ImportExport* format;
    model::Composition* main;
    model::Document* document;
    bool strip;
    QMap<QUuid, int> layer_indices;
    log::Log logger{"Lottie Export"};
    model::Layer* mask = nullptr;
    bool strip_raster;
    bool auto_embed;
    std::unordered_set<model::Group*> wrap_to_layer;
    std::unordered_set<model::Group*> contains_layer;
};



} // namespace glaxnimate::io::lottie::detail
