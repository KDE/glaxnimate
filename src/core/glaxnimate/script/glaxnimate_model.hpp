/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/command/object_list_commands.hpp"

#include "glaxnimate/model/document.hpp"

#include "glaxnimate/model/shapes/composable/group.hpp"
#include "glaxnimate/model/shapes/composable/layer.hpp"
#include "glaxnimate/model/shapes/composable/precomp_layer.hpp"
#include "glaxnimate/model/shapes/composable/image.hpp"

#include "glaxnimate/model/shapes/shapes/rect.hpp"
#include "glaxnimate/model/shapes/shapes/ellipse.hpp"
#include "glaxnimate/model/shapes/shapes/path.hpp"
#include "glaxnimate/model/shapes/shapes/polystar.hpp"

#include "glaxnimate/model/shapes/style/fill.hpp"
#include "glaxnimate/model/shapes/style/stroke.hpp"

#include "glaxnimate/model/shapes/modifiers/repeater.hpp"
#include "glaxnimate/model/shapes/modifiers/trim.hpp"
#include "glaxnimate/model/shapes/modifiers/inflate_deflate.hpp"
#include "glaxnimate/model/shapes/modifiers/round_corners.hpp"
#include "glaxnimate/model/shapes/modifiers/offset_path.hpp"
#include "glaxnimate/model/shapes/modifiers/zig_zag.hpp"

#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/model/assets/named_color.hpp"
#include "glaxnimate/model/assets/composition.hpp"

#include "glaxnimate/script/register_machinery.hpp"

namespace glaxnimate::script {


template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class AddShapeBase
{
public:
    using PtrMem = PropT Owner::*;

    AddShapeBase(PtrMem p) noexcept : ptr(p) {}

protected:
    ItemT* create(model::Document* doc, PropT& prop, const QString& clsname, int index) const
    {
        auto obj = model::Factory::static_build(clsname, doc);
        if ( !obj )
            return nullptr;

        auto cast = obj->cast<ItemT>();

        if ( !cast )
        {
            delete obj;
            return nullptr;
        }

        if constexpr ( std::is_base_of_v<model::DocumentNode, ItemT> )
            doc->set_best_name(static_cast<model::DocumentNode*>(cast));
        else
            cast->name.set(cast->type_name_human());

        doc->push_command(new command::AddObject<ItemT, PropT>(&prop, std::unique_ptr<ItemT>(cast), index));

        return cast;
    }

    PtrMem ptr;
};

template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class AddShapeName : public AddShapeBase<Owner, PropT, ItemT>
{
public:
    using AddShapeBase<Owner, PropT, ItemT>::AddShapeBase;

    ItemT* operator() (Owner* owner, const QString& clsname, int index = -1) const
    {
        return this->create(owner->document(), owner->*(this->ptr), clsname, index);
    }
};

template<class Owner, class PropT, class ItemT = typename PropT::value_type>
class AddShapeClone
{
public:
    using PtrMem = PropT Owner::*;

    AddShapeClone(PtrMem p) noexcept : ptr(p) {}

    ItemT* operator() (Owner* owner, ItemT* object, int index = -1) const
    {
        if ( !object )
            return nullptr;

        std::unique_ptr<ItemT> clone(static_cast<ItemT*>(object->clone().release()));
        if ( clone->document() != owner->document() )
            clone->transfer(owner->document());

        auto ptr = clone.get();

        owner->push_command(new command::AddObject<ItemT, PropT>(&(owner->*(this->ptr)), std::move(clone), index));

        return ptr;
    }

private:

    PtrMem ptr;
};


template<class Reg>
void register_top_level(const typename Reg::module& model)
{
    // TODO define Document, Object, DocumentNode, Composition
    register_from_meta<Reg, model::VisualNode, model::DocumentNode>(model);
    register_from_meta<Reg, model::AnimationContainer, model::Object>(model);
    register_from_meta<Reg, model::StretchableTime, model::Object>(model);
    register_from_meta<Reg, model::Transform, model::Object>(model);
    register_from_meta<Reg, model::MaskSettings, model::Object>(model);
}

template<class Reg>
void register_assets(const typename Reg::module& defs)
{
    // TODO define Asset
    register_from_meta<Reg, model::BrushStyle, model::Asset>(defs);
    register_constructible<Reg, model::NamedColor, model::BrushStyle>(defs);
    register_constructible<Reg, model::GradientColors, model::Asset>(defs);
    register_constructible<Reg, model::Gradient, model::BrushStyle>(defs, enums<model::Gradient::GradientType>{});
    register_constructible<Reg, model::Bitmap, model::Asset>(defs);
    register_from_meta<Reg, model::EmbeddedFont, model::Asset>(defs);
    register_from_meta<Reg, model::BitmapList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::NamedColorList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::GradientList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::GradientColorsList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::CompositionList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::FontList, model::DocumentNode>(defs);
    register_from_meta<Reg, model::Assets, model::DocumentNode>(defs);
}

template<class Reg>
void register_shapes(const typename Reg::module& shapes)
{
    // TODO define ShapeElement
    register_from_meta<Reg, model::Shape, model::ShapeElement>(shapes);
    register_from_meta<Reg, model::Modifier, model::ShapeElement>(shapes);
    register_from_meta<Reg, model::Styler, model::ShapeElement>(shapes);
    register_from_meta<Reg, model::Composable, model::ShapeElement>(shapes, enums<renderer::BlendMode>{});

    register_constructible<Reg, model::Rect, model::Shape>(shapes);
    register_constructible<Reg, model::Ellipse, model::Shape>(shapes);
    register_constructible<Reg, model::PolyStar, model::Shape>(shapes, enums<model::PolyStar::StarType>{});
    register_constructible<Reg, model::Path, model::Shape>(shapes);

    auto cls_group = register_constructible<Reg, model::Group, model::Composable>(shapes);
    Reg::define_add_shape(cls_group);
    register_constructible<Reg, model::Layer, model::Group>(shapes);
    register_constructible<Reg, model::PreCompLayer, model::Composable>(shapes);
    register_constructible<Reg, model::Image, model::Composable>(shapes);

    register_constructible<Reg, model::Fill, model::Styler>(shapes, enums<model::Fill::Rule>{});
    register_constructible<Reg, model::Stroke, model::Styler>(shapes, enums<model::Stroke::Cap, model::Stroke::Join>{});
    register_constructible<Reg, model::Repeater, model::Modifier>(shapes);

    register_from_meta<Reg, model::PathModifier, model::Modifier>(shapes);
    register_constructible<Reg, model::Trim, model::PathModifier>(shapes);
    register_constructible<Reg, model::InflateDeflate, model::PathModifier>(shapes);
    register_constructible<Reg, model::RoundCorners, model::PathModifier>(shapes);
    register_constructible<Reg, model::OffsetPath, model::PathModifier>(shapes);
    register_constructible<Reg, model::ZigZag, model::PathModifier>(shapes);
}

} // namespace glaxnimate::script
