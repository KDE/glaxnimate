/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <set>

#include "glaxnimate/model/animation/animatable_base.hpp"

namespace glaxnimate::model {

class Object;


class MetaKeyframe : public KeyframeBase
{
public:
    MetaKeyframe(FrameTime time, std::set<AnimatableBase*> external = {})
        : KeyframeBase(time), external_(std::move(external))
    {}

    const std::set<AnimatableBase*>& external() const { return external_; }
    bool empty() const { return external_.empty(); }
    int size() const { return external_.size(); }

    void add_property(AnimatableBase* prop)
    {
        external_.insert(prop);
    }

    void remove_property(AnimatableBase* prop)
    {
        external_.erase(prop);
    }

    void set_transition(const KeyframeTransition &trans) override;

    // dummy
    QVariant value() const override { return {}; }
    bool set_value(const QVariant& ) override { return false; }
    KeyframeTransition transition() const override { return KeyframeTransition::Special::NoValue; }

private:
    std::set<AnimatableBase*> external_;

protected:
    std::unique_ptr<KeyframeBase> do_clone() const override;

    class MetaKeyframeSplitter : public KeyframeSplitter
    {
    public:
        MetaKeyframeSplitter(const MetaKeyframe* a, const MetaKeyframe* b) : a(a), b(b) {}

        void step(const QPointF&) override {}

        std::unique_ptr<KeyframeBase> left(const QPointF& p) const override
        {
            return std::make_unique<MetaKeyframe>(
                math::lerp(a->time(), b->time(), p.x()),
                a->external_
            );
        }

        std::unique_ptr<KeyframeBase> right(const QPointF& p) const override
        {
            return std::make_unique<MetaKeyframe>(
                math::lerp(a->time(), b->time(), p.x()),
                a->external_
            );
        }

        std::unique_ptr<KeyframeBase> last() const override { return b->clone(); }

        const MetaKeyframe* a;
        const MetaKeyframe* b;
    };

    std::unique_ptr<KeyframeSplitter> splitter(const KeyframeBase* other) const override
    {
        return std::make_unique<MetaKeyframeSplitter>(this, static_cast<const MetaKeyframe*>(other));
    }
};

/**
 * @brief Animatable that tracks other animatables
 *
 * Used to group keyframes within an object
 */
class MetaAnimatable : public detail::AnimatableImpl<MetaKeyframe, AnimatableBase>
{
    Q_OBJECT

private:
    using container = KeyframeContainer<MetaKeyframe>;

public:
    explicit MetaAnimatable(Object* object);
    ~MetaAnimatable();

    Object* object() const;
    void add_animatable(model::AnimatableBase* prop);
    const std::vector<AnimatableBase*>& animatables() const;

    int animatable_flags() const override { return IsWritable|IsMeta; }


    QUndoCommand* command_add_smooth_keyframe(FrameTime time, const QVariant& value, bool commit = true, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_remove_keyframe(FrameTime time, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_clear_keyframes(QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_set_transition(model::FrameTime time, const model::KeyframeTransition& transition, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_set_transition_side(
        model::FrameTime time,
        model::KeyframeTransition::Descriptive desc,
        const QPointF& point,
        bool before_transition,
        QUndoCommand* parent = nullptr
    ) override;
    QUndoCommand* command_move_keyframe(model::FrameTime time_before, model::FrameTime time_after, QUndoCommand* parent = nullptr) override;

    // Renamed to avoid clashing with BaseProperty


    // void clear_keyframes() override;
    // bool remove_keyframe_at_time(FrameTime time) override;
    // void set_transition(FrameTime time, const KeyframeTransition& transition) override;
    // void set_transition_before(FrameTime time, const KeyframeTransition& transition) override;
    // MoveResult move_keyframe(FrameTime from_time, FrameTime to_time) override;
    // KeyframeBase* set_keyframe(FrameTime time, const QVariant&, SetKeyframeInfo* info, bool) override;

    QString visual_name() const override;

    bool value_mismatch() const override { return false; }
    QVariant value_at_time(FrameTime) const override { return {}; }
    QVariant static_value() const override { return {}; }
    bool set_static_value(const QVariant& ) override { return false; }
    QVariant do_mid_transition_value(const KeyframeBase*, const KeyframeBase*, qreal) const override { return {}; }

private Q_SLOTS:
    void external_keyframe_added(FrameTime time);
    void external_keyframe_removed(FrameTime time);
    void external_keyframe_moved(FrameTime from_time, FrameTime to_time);

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::model
