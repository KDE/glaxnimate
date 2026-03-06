/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/shapes/composable/composable.hpp"

using namespace glaxnimate::model;


Composable::Composable(Document *document)
    : ShapeElement(document)
{
    connect(transform.get(), &Object::property_changed,
            this, &Composable::on_transform_matrix_changed);

}


void glaxnimate::model::Composable::on_paint(renderer::Renderer* painter, glaxnimate::model::FrameTime time, glaxnimate::model::VisualNode::PaintMode, glaxnimate::model::Modifier*) const
{
    painter->set_blend_mode(blend_mode.get());
    painter->set_opacity(opacity.get_at(time));
}

void Composable::on_transform_matrix_changed()
{
    propagate_bounding_rect_changed();
    Q_EMIT local_transform_matrix_changed(local_transform_matrix(time()));
    propagate_transform_matrix_changed(transform_matrix(time()), group_transform_matrix(time()));
}
