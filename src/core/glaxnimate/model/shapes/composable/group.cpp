/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/shapes/composable/group.hpp"

#include <QPainter>

#include "glaxnimate/model/document.hpp"
#include "glaxnimate/model/shapes/style/styler.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Group)


void glaxnimate::model::Group::add_shapes(glaxnimate::model::FrameTime t, math::bezier::MultiBezier & bez, const QTransform& parent_transform) const
{
    QTransform trans = transform.get()->transform_matrix(t) * parent_transform;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        ch->add_shapes(t, bez, trans);
    }
}

QRectF glaxnimate::model::Group::local_bounding_rect(FrameTime t) const
{
    if ( shapes.empty() )
        return owner_composition()->rect();
    return shapes.bounding_rect(t);
}

QTransform glaxnimate::model::Group::local_transform_matrix(glaxnimate::model::FrameTime t) const
{
    return transform.get()->transform_matrix(t);
}


glaxnimate::math::bezier::MultiBezier glaxnimate::model::Group::to_painter_path_impl(glaxnimate::model::FrameTime t) const
{
    glaxnimate::math::bezier::MultiBezier path;
    for ( const auto& ch : utils::Range(shapes.begin(), shapes.past_first_modifier()) )
    {
        if ( ch->is_instance<glaxnimate::model::Styler>() || ch->is_instance<glaxnimate::model::Group>()  )
            path.append(ch->to_clip(t));
    }

    return path;
}


glaxnimate::math::bezier::MultiBezier glaxnimate::model::Group::to_clip(FrameTime t) const
{
    return to_painter_path(t).transformed(transform.get()->transform_matrix(t));
}

std::unique_ptr<glaxnimate::model::ShapeElement> glaxnimate::model::Group::to_path() const
{
    auto clone = std::make_unique<glaxnimate::model::Group>(document());

    for ( BaseProperty* prop : properties() )
    {
        if ( prop != &shapes )
            clone->get_property(prop->name())->assign_from(prop);
    }

    for ( const auto& shape : shapes )
    {
        clone->shapes.insert(shape->to_path());
        if ( shape->is_instance<glaxnimate::model::Modifier>() )
            break;
    }

    return clone;
}

void glaxnimate::model::Group::on_graphics_changed()
{
    ShapeElement::on_graphics_changed();

    for ( const auto& shape : shapes )
    {
        if ( shape->is_instance<glaxnimate::model::ShapeOperator>() )
            shape->on_graphics_changed();
    }

}


void glaxnimate::model::Group::on_composition_changed(model::Composition*, model::Composition* new_comp)
{
    for ( const auto& shape : shapes )
    {
        shape->refresh_owner_composition(new_comp);
    }
}
