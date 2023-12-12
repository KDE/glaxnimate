/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "base.hpp"

#include "command/base.hpp"
#include "model/animation/animatable.hpp"
#include "model/document.hpp"
#include "model/object.hpp"

namespace glaxnimate::command {

class SetKeyframe : public MergeableCommand<Id::SetKeyframe, SetKeyframe>
{
public:
    SetKeyframe(
        model::AnimatableBase* prop,
        model::FrameTime time,
        const QVariant& value,
        bool commit,
        bool force_insert = false
    );

    void undo() override;

    void redo() override;

    bool merge_with(const SetKeyframe& other);

private:
    model::AnimatableBase* prop;
    model::FrameTime time;
    QVariant before;
    QVariant after;
    bool had_before;
    bool calculated = false;
    int insert_index = -1;
    model::KeyframeTransition trans_before;
    model::KeyframeTransition left;
    model::KeyframeTransition right;
    bool force_insert = false;
};

class RemoveKeyframeTime : public QUndoCommand
{
public:
    RemoveKeyframeTime(
        model::AnimatableBase* prop,
        model::FrameTime time
    );

    void undo() override;

    void redo() override;

private:
    model::AnimatableBase* prop;
    model::FrameTime time;
    int index;
    QVariant before;
    model::KeyframeTransition prev_transition_before;
    model::KeyframeTransition prev_transition_after;
};


class RemoveKeyframeIndex: public QUndoCommand
{
public:
    RemoveKeyframeIndex(
        model::AnimatableBase* prop,
        int index
    );

    void undo() override;

    void redo() override;

private:
    model::AnimatableBase* prop;
    int index;
    model::FrameTime time;
    QVariant before;
    model::KeyframeTransition prev_transition_before;
    model::KeyframeTransition prev_transition_after;
};



class RemoveAllKeyframes : public QUndoCommand
{
public:
    RemoveAllKeyframes(model::AnimatableBase* prop, QVariant value);

    void undo() override;

    void redo() override;

private:
    struct Keframe
    {
        model::FrameTime time;
        QVariant value;
        model::KeyframeTransition transition;
    };
    model::AnimatableBase* prop;
    std::vector<Keframe> keyframes;
    QVariant before;
    QVariant after;
};


/**
 * \brief Command that sets multiple animated properties at once,
 * setting keyframes based on the document record_to_keyframe
 */
class SetMultipleAnimated : public MergeableCommand<Id::SetMultipleAnimated, SetMultipleAnimated>
{
public:
    SetMultipleAnimated(model::AnimatableBase* prop, QVariant after, bool commit);

    template<class... Args>
    SetMultipleAnimated(
        const QString& name,
        bool commit,
        const std::vector<model::AnimatableBase*>& props,
        Args... vals
    ) : SetMultipleAnimated(name, props, {}, {QVariant::fromValue(vals)...}, commit)
    {}

    /**
     * \pre props.size() == after.size() && (props.size() == before.size() || before.empty())
     *
     * If before.empty() it will be populated by the properties
     */
    SetMultipleAnimated(
        const QString& name,
        const std::vector<model::AnimatableBase*>& props,
        const QVariantList& before,
        const QVariantList& after,
        bool commit
    );

    SetMultipleAnimated(const QString& name, bool commit);

    void push_property(model::AnimatableBase* prop, const QVariant& after);
    void push_property_not_animated(model::BaseProperty* prop, const QVariant& after);

    void undo() override;

    void redo() override;

    bool merge_with(const SetMultipleAnimated& other);

    const std::vector<model::AnimatableBase*>& properties() const { return props; }

    bool empty() const;

private:
    static QString auto_name(model::AnimatableBase* prop);

    std::vector<model::AnimatableBase*> props;
    QVariantList before;
    QVariantList after;
    std::vector<int> keyframe_before;
    bool keyframe_after;
    model::FrameTime time;
    std::vector<bool> add_0;
    std::vector<model::BaseProperty*> props_not_animated;
};


class SetKeyframeTransition : public QUndoCommand
{
public:
    SetKeyframeTransition(
        model::AnimatableBase* prop,
        int keyframe_index,
        model::KeyframeTransition::Descriptive desc,
        const QPointF& point,
        bool before_transition
    );

    SetKeyframeTransition(
        model::AnimatableBase* prop,
        int keyframe_index,
        const model::KeyframeTransition& transition
    );

    void undo() override;
    void redo() override;

private:
    model::KeyframeBase* keyframe() const;

    model::AnimatableBase* prop;
    int keyframe_index;
    model::KeyframeTransition undo_value;
    model::KeyframeTransition redo_value;
};


class MoveKeyframe : public QUndoCommand
{
public:
    MoveKeyframe(
        model::AnimatableBase* prop,
        int keyframe_index,
        model::FrameTime time_after
    );

    void undo() override;

    void redo() override;

    /**
     * \brief The index after redo()
     * \pre redo() called at least once
     */
    int redo_index() const;

private:
    model::AnimatableBase* prop;
    int keyframe_index_before;
    int keyframe_index_after = -1;
    model::FrameTime time_before;
    model::FrameTime time_after;
};

template<class T>
class StretchTimeCommand: public QUndoCommand
{
public:
    /**
     * \pre multiplier > 0
     */
    StretchTimeCommand(T* target, qreal multiplier)
        : QUndoCommand(i18n("Stretch Time")),
          target(target),
          multiplier(multiplier)
    {}

    void undo() override
    {
        target->stretch_time(1/multiplier);
        if constexpr ( !std::is_same_v<T, model::Document> )
            target->set_time(target->document()->current_time());
    }

    void redo() override
    {
        target->stretch_time(multiplier);
        if constexpr ( !std::is_same_v<T, model::Document> )
            target->set_time(target->document()->current_time());
    }

private:
    T* target;
    qreal multiplier;
};


/**
 * \brief Command that sets the path of an animated position
 */
class SetPositionBezier : public MergeableCommand<Id::SetMultipleAnimated, SetPositionBezier>
{
public:
    SetPositionBezier(model::detail::AnimatedPropertyPosition* prop, math::bezier::Bezier after, bool commit, const QString& name = "");
    SetPositionBezier(model::detail::AnimatedPropertyPosition* prop, math::bezier::Bezier before, math::bezier::Bezier after, bool commit, const QString& name = "");

    void undo() override;

    void redo() override;

    bool merge_with(const SetPositionBezier& other);

private:
    model::detail::AnimatedPropertyPosition* property;
    math::bezier::Bezier before;
    math::bezier::Bezier after;
};


/**
 * \brief Undo command whose children are done and undone in custom order
 */
class ReorderedUndoCommand : public QUndoCommand
{
public:
    using QUndoCommand::QUndoCommand;

    void add_command(std::unique_ptr<QUndoCommand> cmd, int order_redo, int order_undo)
    {
        undo_map[order_undo] = cmd.get();
        redo_map[order_redo] = std::move(cmd);
    }

    void undo() override
    {
        for ( const auto& p : undo_map )
            p.second->undo();
    }

    void redo() override
    {
        for ( const auto& p : redo_map )
            p.second->redo();
    }

private:
    std::map<int, std::unique_ptr<QUndoCommand>> redo_map;
    std::map<int, QUndoCommand*> undo_map;
};

} // namespace glaxnimate::command
