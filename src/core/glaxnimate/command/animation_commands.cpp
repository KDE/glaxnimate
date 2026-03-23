/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/command/animation_commands.hpp"
#include "glaxnimate/command/undo_macro_guard.hpp"

#include "glaxnimate/model/document.hpp"

glaxnimate::command::SetKeyframe::SetKeyframe(
    model::AnimatableBase* prop,
    model::FrameTime time,
    const QVariant& value,
    bool commit,
    QUndoCommand* parent
    ) : Parent(command_name(prop, time), commit, parent),
    prop(prop),
    time(time),
    before(prop->value_at_time(time)),
    after(value),
    had_before(prop->has_keyframe(time))
{}

QString glaxnimate::command::SetKeyframe::command_name(model::AnimatableBase *prop, model::FrameTime time)
{
    return i18n("Update %1 keyframe at %2", prop->visual_name(), time);
}

void glaxnimate::command::SetKeyframe::undo()
{
    if ( had_before )
        prop->set_keyframe(time, before);
    else
        prop->remove_keyframe_at_time(time);

    prop->set_transition_before(time, trans_before);
}

void glaxnimate::command::SetKeyframe::redo()
{
    model::KeyframeBase* kf = nullptr;
    if ( !calculated )
    {
        auto mid = prop->mid_transition(time);
        model::AnimatedPropertyBase::SetKeyframeInfo info;
        kf = prop->set_keyframe(time, after, &info);
        if ( kf && info.insertion )
        {
            if ( mid.type != model::AnimatedPropertyBase::MidTransition::Middle )
            {
                adjust_transition = false;
            }
            else
            {

                auto kf_before = prop->keyframe_before(time);
                adjust_transition = kf_before;
                trans_before = kf_before->transition();

                left = mid.from_previous;
                right = mid.to_next;
            }
        }
    }
    else
    {
        prop->set_keyframe(time, after, nullptr);
    }

    if ( adjust_transition )
    {
        prop->set_transition_before(time, left);
        prop->set_transition(time, right);
    }

}

bool glaxnimate::command::SetKeyframe::merge_with(const SetKeyframe& other)
{
    if ( other.prop != prop )
        return false;
    after = other.after;
    return true;
}

glaxnimate::command::RemoveKeyframeTime::RemoveKeyframeTime(
    model::AnimatableBase* prop,
    model::FrameTime time,
    QUndoCommand* parent
) : QUndoCommand(command_name(prop, time), parent),
    prop(prop),
    time(time)
{
    auto kf = prop->keyframe_at(time);
    before = kf->value();
    transition_before = kf->transition();

    if ( auto kf_before = prop->keyframe_before(time) )
    {
        prev_transition_after = prev_transition_before = kf_before->transition();
        if ( !prev_transition_after.hold() )
            prev_transition_after.set_after(kf->transition().after());
    }
}

QString glaxnimate::command::RemoveKeyframeTime::command_name(model::AnimatableBase *prop, model::FrameTime time)
{
    return i18n("Remove %1 keyframe at %2", prop->visual_name(), time);
}

void glaxnimate::command::RemoveKeyframeTime::undo()
{
    prop->set_keyframe(time, before)->set_transition(transition_before);
    prop->set_transition_before(time, prev_transition_before);
}

void glaxnimate::command::RemoveKeyframeTime::redo()
{
    prop->set_transition_before(time, prev_transition_after);
    prop->remove_keyframe_at_time(time);
}

glaxnimate::command::SetMultipleAnimated::SetMultipleAnimated(model::AnimatedPropertyBase* prop, QVariant after, bool commit)
    : SetMultipleAnimated(
        auto_name(prop),
        {prop},
        {},
        {after},
        commit,
        prop->time(),
        prop->object()->document()->record_to_keyframe()
    )
{}

glaxnimate::command::SetMultipleAnimated::SetMultipleAnimated(
    const QString& name,
    const std::vector<model::AnimatableBase*>& props,
    const QVariantList& before,
    const QVariantList& after,
    bool commit,
    model::FrameTime time,
    bool record_to_keyframe
)
    : Parent(name, commit),
    props(props),
    before(before),
    after(after),
    keyframe_after_global(record_to_keyframe),
    time(time)
{
    bool add_before = before.empty();

    for ( auto prop : props )
    {
        if ( add_before )
            this->before.push_back(prop->static_value());
        keyframe_before.push_back(prop->has_keyframe(time));
        keyframe_after.push_back(prop->animated());
        add_0.push_back(time != 0 && !prop->animated() && keyframe_after_global);
    }
}


glaxnimate::command::SetMultipleAnimated::SetMultipleAnimated(const QString& name, bool commit)
    : Parent(name, commit), keyframe_after_global(false)
{
}

void glaxnimate::command::SetMultipleAnimated::push_property(model::AnimatedPropertyBase* prop, const QVariant& after_val)
{
    keyframe_after_global = keyframe_after_global || prop->object()->document()->record_to_keyframe();
    keyframe_after.push_back(prop->animated());
    time = prop->time();
    int insert = props.size();
    props.push_back(prop);
    before.insert(before.begin() + insert, prop->value());
    after.insert(after.begin() + insert, after_val);
    keyframe_before.push_back(prop->has_keyframe(time));
    add_0.push_back(!prop->animated() && keyframe_after_global);
}

void glaxnimate::command::SetMultipleAnimated::push_property_not_animated(model::BaseProperty* prop, const QVariant& after_val)
{
    props_not_animated.push_back(prop);
    before.push_back(prop->value());
    after.push_back(after_val);
}

void glaxnimate::command::SetMultipleAnimated::undo()
{
    for ( int i = 0; i < int(props.size()); i++ )
    {
        auto prop = props[i];

        if ( add_0[i] )
            prop->remove_keyframe_at_time(0);

        if ( keyframe_after_global || keyframe_after[i] )
        {
            if ( keyframe_before[i] )
            {
                prop->set_keyframe(time, before[i]);
            }
            else
            {
                prop->remove_keyframe_at_time(time);
                prop->set_static_value(before[i]);
            }
        }
        else
        {
            if ( keyframe_before[i] )
                prop->set_keyframe(time, before[i]);
            else if ( !prop->animated() )
                prop->set_static_value(before[i]);
        }

    }

    for ( int i = 0; i < int(props_not_animated.size()); i++ )
    {
        props_not_animated[i]->set_value(before[i+props.size()]);
    }
}

void glaxnimate::command::SetMultipleAnimated::redo()
{
    for ( int i = 0; i < int(props.size()); i++ )
    {
        auto prop = props[i];

        if ( add_0[i] )
            prop->set_keyframe(0, before[i]);

        if ( keyframe_after_global || keyframe_after[i] )
            prop->set_keyframe(time, after[i]);
        else
            prop->set_static_value(after[i]);
    }

    for ( int i = 0; i < int(props_not_animated.size()); i++ )
    {
        props_not_animated[i]->set_value(after[i+props.size()]);
    }
}


bool glaxnimate::command::SetMultipleAnimated::merge_with(const SetMultipleAnimated& other)
{
    if ( other.props.size() != props.size() || keyframe_after != other.keyframe_after ||
        time != other.time || other.props_not_animated.size() != props_not_animated.size())
        return false;

    for ( int i = 0; i < int(props.size()); i++ )
        if ( props[i] != other.props[i] )
            return false;

    for ( int i = 0; i < int(props_not_animated.size()); i++ )
        if ( props_not_animated[i] != other.props_not_animated[i] )
            return false;

    after = other.after;
    return true;
}

QString glaxnimate::command::SetMultipleAnimated::auto_name(model::AnimatedPropertyBase* prop)
{
    bool key_before = prop->has_keyframe(prop->time());
    bool key_after = prop->object()->document()->record_to_keyframe();

    if ( key_after && !key_before )
        return i18n("Add keyframe for %1 at %2", prop->name(), prop->time());

    if ( key_before )
        return i18n("Update %1 at %2", prop->name(), prop->time());

    return i18n("Update %1", prop->name());
}

bool glaxnimate::command::SetMultipleAnimated::empty() const
{
    return props.empty() && props_not_animated.empty();
}

glaxnimate::command::SetKeyframeTransition::SetKeyframeTransition(
        model::AnimatableBase* prop,
        model::FrameTime time,
        const model::KeyframeTransition& transition,
        QUndoCommand *parent
    )
: QUndoCommand(i18n("Update keyframe transition"), parent),
    prop(prop),
    time(time),
    undo_value(prop->keyframe_at(time)->transition()),
    redo_value(transition)
{
}

void glaxnimate::command::SetKeyframeTransition::undo()
{
    prop->set_transition(time, undo_value);
}

void glaxnimate::command::SetKeyframeTransition::redo()
{
    prop->set_transition(time, redo_value);
}

glaxnimate::model::KeyframeTransition glaxnimate::command::SetKeyframeTransition::transition_side(
    model::AnimatableBase *prop,
    model::FrameTime time,
    model::KeyframeTransition::Descriptive desc,
    const QPointF &point,
    bool before_transition)
{
    auto transition = prop->keyframe_at(time)->transition();

    if ( desc == model::KeyframeTransition::Custom )
    {
        if ( before_transition )
            transition.set_before(point);
        else
            transition.set_after(point);
    }
    else
    {
        if ( before_transition )
            transition.set_before_descriptive(desc);
        else
            transition.set_after_descriptive(desc);
    }

    return transition;
}

glaxnimate::command::MoveKeyframe::MoveKeyframe(
    model::AnimatableBase* prop,
    model::FrameTime time_before,
    model::FrameTime time_after,
    QUndoCommand* parent
) : QUndoCommand(i18n("Move keyframe"), parent),
    prop(prop),
    time_before(time_before),
    time_after(time_after)
{}

void glaxnimate::command::MoveKeyframe::undo()
{
    prop->move_keyframe(time_after, time_before);
}

void glaxnimate::command::MoveKeyframe::redo()
{
    prop->move_keyframe(time_before, time_after);
}


glaxnimate::command::RemoveAllKeyframes::RemoveAllKeyframes(model::AnimatableBase* prop, QVariant after, QUndoCommand *parent)
    : QUndoCommand(command_name(prop), parent),
      prop(prop),
      before(prop->static_value()),
      after(std::move(after))
{
    int count = prop->keyframe_count();
    keyframes.reserve(count);
    for ( const auto& kf : prop->keyframe_range() )
    {
        keyframes.push_back({
            kf.time(),
            kf.value(),
            kf.transition()
        });
    }
}

QString glaxnimate::command::RemoveAllKeyframes::command_name(model::AnimatableBase *prop)
{
    return i18n("Remove animations from %1", prop->visual_name());
}

void glaxnimate::command::RemoveAllKeyframes::redo()
{
    prop->clear_keyframes();
    prop->set_static_value(after);
}

void glaxnimate::command::RemoveAllKeyframes::undo()
{
    for ( const auto& kf : keyframes )
    {
        prop->set_keyframe(kf.time, kf.value, nullptr, true)->set_transition(kf.transition);
    }
    // prop->set_time(prop->time());
    prop->set_static_value(before);
}


glaxnimate::command::SetPositionBezier::SetPositionBezier(
    model::detail::AnimatedPropertyPosition* prop,
    math::bezier::Bezier after,
    bool commit,
    const QString& name
)
    : SetPositionBezier(prop, prop->bezier(), std::move(after), commit, name)
{
}

glaxnimate::command::SetPositionBezier::SetPositionBezier(
    model::detail::AnimatedPropertyPosition* prop,
    math::bezier::Bezier before,
    math::bezier::Bezier after,
    bool commit,
    const QString& name
) : Parent(name.isEmpty() ? i18n("Update animation path") : name, commit),
    property(prop),
    before(std::move(before)),
    after(std::move(after))
{
}

bool glaxnimate::command::SetPositionBezier::merge_with(const glaxnimate::command::SetPositionBezier& other)
{
    return property == other.property;
}

void glaxnimate::command::SetPositionBezier::undo()
{
    property->set_bezier(before);
}

void glaxnimate::command::SetPositionBezier::redo()
{
    property->set_bezier(after);
}
