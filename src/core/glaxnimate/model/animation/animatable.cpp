/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/animation/animatable.hpp"

#include "glaxnimate/command/animation_commands.hpp"
#include "glaxnimate/model/object.hpp"
#include "glaxnimate/math/bezier/segment.hpp"

class glaxnimate::model::Keyframe<QPointF>::PointKeyframeSplitter : public KeyframeSplitter
{
public:
    const Keyframe* self;
    const Keyframe* other;
    math::bezier::CubicBezierSolver<QPointF> solver;
    math::bezier::LengthData len;
    QPointF tan_in;
    math::bezier::Point point_before;
    math::bezier::Point point_mid;
    qreal prev_split = 0;
    bool linear;

    PointKeyframeSplitter(const Keyframe<QPointF>* self, const Keyframe<QPointF>* other)
        : self(self),
          other(other),
          solver(self->bezier_solver(*other)),
          len(solver, 20),
          tan_in(self->point_.tan_in),
          linear(self->is_linear())
    {
    }

    void step(const QPointF& p) override
    {
        if ( linear )
            return;

        // TODO: would need the ability to access other keyframes to properly handle values outside [0, 1]
        qreal split = math::bound(0., p.y(), 1.);
        auto beziers = solver.split((split - prev_split) / (1 - prev_split));
        prev_split = split;
        solver = beziers.second;
        point_before = math::bezier::Point (beziers.first[0], tan_in, beziers.first[1]);
        point_mid = math::bezier::Point (beziers.first[3], beziers.first[2], beziers.second[1]);
        tan_in = beziers.second[2];
    }

    std::unique_ptr<KeyframeBase> left(const QPointF& p) const override
    {
        if ( linear )
        {
            return std::make_unique<Keyframe>(
                math::lerp(self->time(), other->time(), p.x()),
                math::lerp(self->get(), other->get(), p.y())
            );
        }

        return std::make_unique<Keyframe>(math::lerp(self->time(), other->time(), p.x()), point_before);
    }

    std::unique_ptr<KeyframeBase> right(const QPointF& p) const override
    {
        if ( linear )
        {
            return std::make_unique<Keyframe>(
                math::lerp(self->time(), other->time(), p.x()),
                math::lerp(self->get(), other->get(), p.y())
            );
        }

        return std::make_unique<Keyframe>(math::lerp(self->time(), other->time(), p.x()), point_mid);
    }

    std::unique_ptr<KeyframeBase> last() const override
    {
        if ( linear )
            return other->clone();

        math::bezier::Point point_after = other->point();
        point_after.tan_in = tan_in;
        return std::make_unique<Keyframe>(other->time(), point_after);
    }

};

std::unique_ptr<glaxnimate::model::KeyframeBase::KeyframeSplitter> glaxnimate::model::Keyframe<QPointF>::splitter(const KeyframeBase* other) const
{
    return std::make_unique<PointKeyframeSplitter>(this, static_cast<const Keyframe<QPointF>*>(other));
}

bool glaxnimate::model::AnimatableBase::assign_from(const model::BaseProperty* prop)
{
    if ( prop->traits().flags != traits().flags || prop->traits().type != traits().type )
        return false;

    const AnimatableBase* other = static_cast<const AnimatableBase*>(prop);

    clear_keyframes();

    if ( !other->animated() )
        return set_value(other->value());

    for ( const auto& kf_other : other->keyframe_range() )
    {
        KeyframeBase* kf = set_keyframe(kf_other.time(), kf_other.value());
        if ( kf )
            kf->set_transition(kf_other.transition());
    }

    return true;
}

bool glaxnimate::model::AnimatableBase::set_undoable(const QVariant& val, bool commit)
{
    if ( !valid_value(val) )
        return false;

    object()->push_command(new command::SetMultipleAnimated(
        i18n("Update %1", name()),
        {this},
        {value()},
        {val},
        commit
    ));
    return true;
}

glaxnimate::model::AnimatableBase::MidTransition glaxnimate::model::AnimatableBase::mid_transition(model::FrameTime time) const
{
    const KeyframeBase* kf_during = this->keyframe_containing(time);
    if ( !kf_during )
        return {MidTransition::Invalid, value(), {}, {}};

    auto before_time = kf_during->time();

    if ( before_time >= time )
        return {MidTransition::SingleKeyframe, kf_during->value(), {}, kf_during->transition(),};


    const KeyframeBase* kf_after = this->keyframe_after(time);

    if ( !kf_after )
        return {MidTransition::SingleKeyframe, kf_during->value(), kf_during->transition(), {},};

    auto after_time = kf_after->time();

    if ( after_time <= time )
        return {
            MidTransition::SingleKeyframe,
            kf_after->value(),
            kf_during->transition(),
            kf_after->transition(),
        };

    qreal x = math::unlerp(before_time, after_time, time);
    return do_mid_transition(kf_during, kf_after, x);
}

glaxnimate::model::AnimatableBase::MidTransition glaxnimate::model::AnimatableBase::do_mid_transition(
    const model::KeyframeBase* kf_during,
    const model::KeyframeBase* kf_after,
    qreal x
) const
{
    const auto& beftrans = kf_during->transition();
    if ( beftrans.hold() || (beftrans.before() == QPointF(0, 0) && beftrans.after() == QPointF(1,1)) )
        return {MidTransition::Middle, kf_during->value(), beftrans, beftrans};

    qreal t = beftrans.bezier_parameter(x);

    if ( t <= 0 )
    {
        KeyframeTransition from_previous = {{}, {1, 1}};
        if ( auto kf_before = keyframe_before(kf_during->time()) )
            from_previous = kf_before->transition();

        return {MidTransition::SingleKeyframe, kf_during->value(), from_previous, beftrans};
    }
    else if ( t >= 1 )
    {
        return {MidTransition::SingleKeyframe, kf_during->value(), beftrans, kf_after->transition(),};
    }


    model::AnimatableBase::MidTransition mt;
    mt.type = MidTransition::Middle;
    mt.value = do_mid_transition_value(kf_during, kf_after, x);
    std::tie(mt.from_previous, mt.to_next) = beftrans.split(x);
    return mt;
}

void glaxnimate::model::AnimatableBase::clear_keyframes_undoable(QVariant value)
{
    if ( !value.isValid() || value.isNull() )
        value = this->value();

    object()->push_command(new command::RemoveAllKeyframes(this, std::move(value)));
}

void glaxnimate::model::AnimatableBase::add_smooth_keyframe_undoable(FrameTime time, const QVariant& val)
{
    object()->push_command(
        new command::SetKeyframe(this, time, val.isNull() ? value() : val, true)
    );
}

void glaxnimate::model::detail::AnimatedPropertyPosition::split_segment(int index, qreal factor)
{
    if ( keyframes_.size() < 2 )
        return;

    auto before = bezier();
    auto after = before;
    after.split_segment(index, factor);

    auto parent = std::make_unique<command::ReorderedUndoCommand>(i18n("Split Segment"));

    FrameTime time = 0;
    QVariant value;

    if ( index <= 0 && factor <= 0 )
    {
        time = keyframes_.begin()->time();
        value = keyframes_.begin()->value();
    }
    else if ( index >= int(keyframes_.size()) - 1 && factor >= 1 )
    {
        auto back = keyframes_.end();
        --back;
        time = back->time();
        value = back->value();
    }
    else
    {
        auto iter = keyframes_.begin();
        std::advance(iter, index);
        auto kf_before = iter;
        ++iter;
        auto kf_after = iter;

        value = kf_before->lerp(*kf_after, factor);

        // Reverse length.at_ratio() to get the correct time at which the transition is equal to `value`
        math::bezier::Solver segment(kf_before->get(), kf_before->point().tan_out, kf_after->point().tan_in, kf_after->get());
        math::bezier::LengthData length(segment, 20);
        qreal time_factor = qFuzzyIsNull(length.length()) ? 0 : length.from_ratio(factor) / length.length();
        time = qRound(math::lerp(kf_before->time(), kf_after->time(), time_factor));
    }

    parent->add_command(
        std::make_unique<command::SetKeyframe>(this, time, value, true, true),
        0, 0
    );

    parent->add_command(
        std::make_unique<command::SetPositionBezier>(this, before, after, true),
        1, 1
    );

    object()->push_command(parent.release());
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::set_bezier(math::bezier::Bezier bezier)
{
    bezier.add_close_point();

    // TODO if sizes don't match, re-arrange keyframes based on
    // how far keyframes are in the bezier
    // eg: a point at 50% of the length will result in a keyframe
    // at time (keyframes[0].time + keyframes[-1].time) / 2
    if ( bezier.size() != int(keyframes_.size()) )
        return false;

    int i = 0;
    for ( auto kf = keyframes_.begin(); kf != keyframes_.end(); ++kf )
    {
        kf->set_point(bezier[i]);
        on_keyframe_updated(kf);
        i++;
    }

    value_ = get_at_impl(time());
    emitter(this->object(), value_);
    Q_EMIT bezier_set(bezier);

    return true;
}

void glaxnimate::model::detail::AnimatedPropertyPosition::remove_points(const std::set<int>& indices)
{
    auto parent = std::make_unique<command::ReorderedUndoCommand>(i18n("Remove Nodes"));

    auto before = bezier();
    auto after = before.removed_points(indices);

    int order = 0;
    int index = 0;
    auto iter = keyframes_.begin();
    while ( iter != keyframes_.end() )
    {
        if ( indices.count(index) )
        {
            auto time = iter->time();
            ++iter;
            parent->add_command(std::make_unique<command::RemoveKeyframeTime>(this, time), -order, order);
            ++order;
        }
        else
        {
            ++iter;
        }
        ++index;
    }

    object()->push_command(parent.release());
}

glaxnimate::math::bezier::Bezier glaxnimate::model::detail::AnimatedPropertyPosition::bezier() const
{
    math::bezier::Bezier bez;
    for ( const auto& kf : keyframes_ )
        bez.push_back(kf.point());

    return bez;
}

glaxnimate::model::detail::AnimatedPropertyPosition::keyframe_type*
    glaxnimate::model::detail::AnimatedPropertyPosition::set_keyframe(
        FrameTime time, const QVariant& val, SetKeyframeInfo* info, bool force_insert
)
{
    if ( val.userType() == QMetaType::QPointF )
        return detail::AnimatedProperty<QPointF>::set_keyframe(time, val.value<QPointF>(), info, force_insert);

    if ( auto v = detail::variant_cast<math::bezier::Point>(val) )
    {
        auto kf = detail::AnimatedProperty<QPointF>::set_keyframe(time, v->pos, info, force_insert);
        kf->set_point(*v);
        Q_EMIT bezier_set(bezier());
        return kf;
    }

    // We accept a bezier here so it can be used with SetMultipleAnimated
    if ( auto v = detail::variant_cast<math::bezier::Bezier>(val) )
    {
        set_bezier(*v);
        return nullptr;
    }

    return nullptr;
}

glaxnimate::model::detail::AnimatedPropertyPosition::keyframe_type*
    glaxnimate::model::detail::AnimatedPropertyPosition::set_keyframe(
        FrameTime time, reference value, SetKeyframeInfo* info, bool force_insert
)
{
    return detail::AnimatedProperty<QPointF>::set_keyframe(time, value, info, force_insert);
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::set_value(const QVariant& val)
{
    if ( auto v = detail::variant_cast<QPointF>(val) )
        return detail::AnimatedProperty<QPointF>::set(*v);

    if ( auto v = detail::variant_cast<math::bezier::Bezier>(val) )
        return set_bezier(*v);

    return false;
}

bool glaxnimate::model::detail::AnimatedPropertyPosition::valid_value(const QVariant& val) const
{
    if ( detail::variant_cast<QPointF>(val) || detail::variant_cast<math::bezier::Bezier>(val) )
        return true;
    return false;
}

void glaxnimate::model::detail::AnimatedPropertyPosition::add_smooth_keyframe_undoable(FrameTime time, const QVariant& val)
{
    auto parent = std::make_unique<command::ReorderedUndoCommand>(i18n("Add Keyframe"));

    auto value = val.isNull() ? QVariant(value_) : val;

    parent->add_command(std::make_unique<command::SetKeyframe>(this, time, value, true), 0, 0);

    int count = keyframes_.size();

    if ( value.userType() == QMetaType::QPointF && count >= 2  )
    {
        auto first = keyframes_.find_best(time);
        if ( first->time() < time )
        {
            auto second = first;
            ++second;

            if ( second != keyframes_.end() && (!first->is_linear() || second->is_linear()) )
            {
                double scaled_time = (time - first->time()) / (second->time() - first->time());

                auto factor = first->transition().lerp_factor(scaled_time);
                auto solver = first->bezier_solver(*second);
                math::bezier::LengthData len(solver, 20);
                auto t = len.at_ratio(factor).ratio;
                auto split = solver.split(t);

                auto before = bezier();
                auto after = before;
                int index = std::distance(keyframes_.begin(), first);
                after[index].tan_out = split.first[1];
                after[index+1].tan_in = split.second[2];
                math::bezier::Point p(split.first[3], split.first[2], split.second[1]);
                p.translate_to(value.value<QPointF>());
                after.insert_point(index+1, p);

                parent->add_command(std::make_unique<command::SetPositionBezier>(this, before, after, true), 1, 1);
            }
        }
    }

    object()->document()->push_command(parent.release());
}

std::optional<QPointF> glaxnimate::model::detail::AnimatedPropertyPosition::derivative_at(glaxnimate::model::FrameTime time) const
{
    int count = keyframe_count();
    if ( count < 2 )
        return {};

    auto kf_before = keyframes_.find_best(time);
    auto kf_after = kf_before;
    ++kf_after;
    qreal factor = 1;

    if ( kf_after == keyframes_.end() )
    {
        kf_after = kf_before;
        --kf_before;
    }
    else
    {
        factor = math::unlerp(kf_before->time(), kf_after->time(), time);
    }

    const math::bezier::Point& point_before = kf_before->point();
    const math::bezier::Point& point_after = kf_after->point();
    math::bezier::CubicBezierSolver solver(point_before.pos, point_before.tan_out, point_after.tan_in, point_after.pos);
    return QPointF(
        solver.derivative(factor, 0),
        solver.derivative(factor, 1)
    );
}
