/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <vector>
#include "glaxnimate/model/animation/animatable_base.hpp"

namespace glaxnimate::gui {

struct SelectedKeyframe
{
    model::AnimatableBase* property;
    model::FrameTime time;
};

using KeyframeSelection = std::vector<SelectedKeyframe>;

} // namespace glaxnimate::gui
