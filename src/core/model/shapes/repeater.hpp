/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "shape.hpp"

#include "model/transform.hpp"
#include "model/property/sub_object_property.hpp"

namespace glaxnimate::model {

class Repeater : public StaticOverrides<Repeater, Modifier>
{
    GLAXNIMATE_OBJECT(Repeater)
    GLAXNIMATE_SUBOBJECT(Transform, transform)
    GLAXNIMATE_ANIMATABLE(int, copies, 1)
    GLAXNIMATE_ANIMATABLE(float, start_opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)
    GLAXNIMATE_ANIMATABLE(float, end_opacity, 1, {}, 0, 1, false, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    std::unique_ptr<ShapeElement> to_path() const override;

    int max_copies() const;

protected:
    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;
    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier*) const override;
    bool process_collected() const override;

};

} // namespace glaxnimate::model
