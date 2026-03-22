/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/animatable_base.hpp"
#include "glaxnimate/command/animation_commands.hpp"

QUndoCommand *glaxnimate::model::AnimatableBase::add_smooth_keyframe_command(FrameTime time, const QVariant &value)
{
    return new command::SetKeyframe(this, time, value.isNull() ? static_value() : value, true);
}
