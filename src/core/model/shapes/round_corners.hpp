/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "path_modifier.hpp"

namespace glaxnimate::model {

class RoundCorners : public StaticOverrides<RoundCorners, PathModifier>
{
    GLAXNIMATE_OBJECT(RoundCorners)
    GLAXNIMATE_ANIMATABLE(float, radius, 0, {}, 0,)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();

    math::bezier::MultiBezier process(FrameTime t, const math::bezier::MultiBezier& mbez) const override;

protected:
    bool process_collected() const override;

};

} // namespace glaxnimate::model

