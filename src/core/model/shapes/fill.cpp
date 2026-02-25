/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "fill.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Fill)

void glaxnimate::model::Fill::on_paint(renderer::Renderer* p, glaxnimate::model::FrameTime t, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier* modifier) const
{
    p->set_fill({brush(t), opacity.get_at(t), Qt::FillRule(fill_rule.get())});

    math::bezier::MultiBezier bez;
    if ( modifier )
        bez = modifier->collect_shapes_from(affected(), t, {});
    else
        bez = collect_shapes(t, {});

    p->draw_path(bez);
}

QPainterPath glaxnimate::model::Fill::to_painter_path_impl(glaxnimate::model::FrameTime t) const
{
    return collect_shapes(t, {}).painter_path();
}

