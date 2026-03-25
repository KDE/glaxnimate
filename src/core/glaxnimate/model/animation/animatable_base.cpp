/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/animatable_base.hpp"
#include "glaxnimate/command/animation_commands.hpp"
#include "glaxnimate/model/animation/animatable.hpp"

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_add_smooth_keyframe(FrameTime time, const QVariant &value, bool commit, QUndoCommand *parent)
{
    return new command::SetKeyframe(this, time, value.isNull() ? static_value() : value, commit, parent);
}

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_remove_keyframe(FrameTime time, QUndoCommand *parent)
{
    if ( !keyframe_at(time) )
        return nullptr;
    return new command::RemoveKeyframeTime(this, time, parent);
}

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_clear_keyframes(QUndoCommand *parent)
{
    return new command::RemoveAllKeyframes(this, static_value(), parent);
}

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_set_transition(FrameTime time, const model::KeyframeTransition &transition, QUndoCommand *parent)
{
    if ( !keyframe_at(time) )
        return nullptr;
    return new command::SetKeyframeTransition(this, time, transition, parent);
}

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_set_transition_side(FrameTime time, model::KeyframeTransition::Descriptive desc, const QPointF &point, bool before_transition, QUndoCommand *parent)
{
    if ( !keyframe_at(time) )
        return nullptr;
    auto transition = command::SetKeyframeTransition::transition_side(this, time, desc, point, before_transition);
    return new command::SetKeyframeTransition(this, time, transition, parent);
}

QUndoCommand *glaxnimate::model::AnimatedPropertyBase::command_move_keyframe(FrameTime time_before, FrameTime time_after, QUndoCommand *parent)
{
    if ( !keyframe_at(time_before) )
        return nullptr;

    if ( !keyframe_at(time_after) )
    {
        return new command::MoveKeyframe(this, time_before, time_after, parent);
    }

    auto subp = new QUndoCommand(i18n("Move keyframe"), parent);
    new command::RemoveKeyframeTime(this, time_after, subp);
    new command::MoveKeyframe(this, time_before, time_after, subp);
    return subp;
}
