/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/shapes/shape.hpp"

#include "glaxnimate/model/transform.hpp"
#include "glaxnimate/model/property/sub_object_property.hpp"

namespace glaxnimate::model {

class Composable: public ShapeElement
{
    Q_OBJECT

    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, &Composable::opacity_changed, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_PROPERTY(renderer::BlendMode, blend_mode, renderer::BlendMode::Normal, &Composable::blend_mode_changed, {}, PropertyTraits::Visual|PropertyTraits::Hidden)

public:
    Composable(Document* document);

protected:
    void on_paint(renderer::Renderer*, FrameTime, PaintMode, model::Modifier*) const override;

Q_SIGNALS:
    void opacity_changed(float op);
    void blend_mode_changed(renderer::BlendMode mode);

private Q_SLOTS:
    void on_transform_matrix_changed();

};

} // namespace glaxnimate::model
