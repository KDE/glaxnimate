/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/meta_animatable.hpp"
#include "glaxnimate/model/object.hpp"
#include "glaxnimate/utils/pseudo_mutex.hpp"
#include "glaxnimate/command/animation_commands.hpp"


void glaxnimate::model::MetaKeyframe::set_transition(const KeyframeTransition &)
{
}

std::unique_ptr<glaxnimate::model::KeyframeBase> glaxnimate::model::MetaKeyframe::do_clone() const
{
    auto clone = std::make_unique<MetaKeyframe>(time());
    clone->external_ = external_;
    return clone;
}


class glaxnimate::model::MetaAnimatable::Private
{
public:
    Private(Object* object) : object(object) {}

    std::vector<AnimatableBase*> props;
    Object* object;
    utils::PseudoMutex updating;
};

glaxnimate::model::MetaAnimatable::MetaAnimatable(Object *object)
    : d(std::make_unique<Private>(object))
{
}

glaxnimate::model::MetaAnimatable::~MetaAnimatable() = default;

glaxnimate::model::Object *glaxnimate::model::MetaAnimatable::object() const
{
    return d->object;
}

void glaxnimate::model::MetaAnimatable::add_animatable(AnimatableBase *prop)
{
    connect(prop, &AnimatableBase::keyframe_added, this, &MetaAnimatable::external_keyframe_added);
    connect(prop, &AnimatableBase::keyframe_removed, this, &MetaAnimatable::external_keyframe_removed);
    connect(prop, &AnimatableBase::keyframe_moved, this, &MetaAnimatable::external_keyframe_moved);
    d->props.push_back(prop);
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_add_smooth_keyframe(FrameTime time, const QVariant &, bool, QUndoCommand* parent)
{
    if ( !d->props.size() )
        return nullptr;

    auto cmd = new QUndoCommand(command::SetKeyframe::command_name(this, time), parent);

    for ( auto prop: d->props )
        prop->command_add_smooth_keyframe(time, prop->value_at_time(time), cmd);

    return cmd;
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_remove_keyframe(FrameTime time, QUndoCommand* parent)
{
    if ( !d->props.size() )
        return nullptr;

    auto cmd = new QUndoCommand(command::SetKeyframe::command_name(this, time), parent);

    for ( auto prop: d->props )
        prop->command_remove_keyframe(time, cmd);

    return cmd;
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_clear_keyframes(QUndoCommand* parent)
{
    if ( !d->props.size() )
        return nullptr;

    auto cmd = new QUndoCommand(command::RemoveAllKeyframes::command_name(this), parent);

    for ( auto prop: d->props )
        prop->command_clear_keyframes(cmd);

    return cmd;
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_set_transition(FrameTime time, const model::KeyframeTransition &transition, QUndoCommand* parent)
{
    if ( !d->props.size() )
        return nullptr;

    auto kf = keyframes_.find(time);
    if ( kf == keyframes_.end() )
        return nullptr;

    auto cmd = new QUndoCommand(i18n("Set keyframe transition"), parent);

    for ( auto prop: kf->external() )
        prop->command_set_transition(time, transition, cmd);

    return cmd;
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_set_transition_side(FrameTime time, model::KeyframeTransition::Descriptive desc, const QPointF &point, bool before_transition, QUndoCommand* parent)
{
    if ( !d->props.size() )
        return nullptr;

    auto kf = keyframes_.find(time);
    if ( kf == keyframes_.end() )
        return nullptr;

    auto cmd = new QUndoCommand(i18n("Set keyframe transition"), parent);

    for ( auto prop: kf->external() )
        prop->command_set_transition_side(time, desc, point, before_transition, cmd);

    return cmd;
}

QUndoCommand* glaxnimate::model::MetaAnimatable::command_move_keyframe(FrameTime time_before, FrameTime time_after, QUndoCommand* parent)
{
    qDebug() << visual_name() << time_before << time_after;
    if ( !d->props.size() )
        return nullptr;

    auto kf = keyframes_.find(time_before);
    if ( kf == keyframes_.end() )
        return nullptr;

    auto cmd = new QUndoCommand(i18n("Move Keyframe"), parent);

    for ( auto prop: kf->external() )
        prop->command_move_keyframe(time_before, time_after, cmd);

    return cmd;

}

const std::vector<glaxnimate::model::AnimatableBase *> &glaxnimate::model::MetaAnimatable::animatables() const
{
    return d->props;
}

QString glaxnimate::model::MetaAnimatable::visual_name() const
{
    return d->object->object_name();
}

void glaxnimate::model::MetaAnimatable::external_keyframe_added(FrameTime time)
{
    if ( d->updating )
        return;

    auto prop = static_cast<AnimatableBase*>(sender());
    auto kf = keyframes_.find(time);
    if ( kf != keyframes_.end() )
    {
        kf->add_property(prop);
    }
    else
    {
        keyframes_.insert(time, MetaKeyframe(time, {prop}));
        Q_EMIT keyframe_added(time);
    }
}

void glaxnimate::model::MetaAnimatable::external_keyframe_removed(FrameTime time)
{
    if ( d->updating )
        return;

    auto kf = keyframes_.find(time);
    if ( kf == keyframes_.end() )
        return;

    auto prop = static_cast<AnimatableBase*>(sender());
    kf->remove_property(prop);
    if ( kf->empty() )
    {
        keyframes_.erase(kf);
        Q_EMIT keyframe_removed(time);
    }
}

void glaxnimate::model::MetaAnimatable::external_keyframe_moved(FrameTime from_time, FrameTime to_time)
{
    if ( d->updating )
        return;

    auto kf = keyframes_.find(from_time);
    if ( kf == keyframes_.end() )
        return;

    if ( kf->size() == 1 )
    {
        kf->set_time(to_time);
        keyframes_.move(kf, to_time);
        Q_EMIT keyframe_moved(from_time, to_time);
        return;
    }

    auto prop = static_cast<AnimatableBase*>(sender());

    kf->remove_property(prop);
    external_keyframe_added(to_time);
}



