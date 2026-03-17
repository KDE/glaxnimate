/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/shapes/modifiers/path_modifier.hpp"

namespace glaxnimate::model {

class InflateDeflate : public StaticOverrides<InflateDeflate, PathModifier>
{
    GLAXNIMATE_OBJECT(InflateDeflate)
    GLAXNIMATE_ANIMATABLE(float, amount, 0, {}, -1, 1, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;

protected:
    bool process_collected() const override;

};

} // namespace glaxnimate::model
