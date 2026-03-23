/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QUndoCommand>

#include "glaxnimate/model/animation/keyframe_base.hpp"
#include "glaxnimate/model/animation/keyframe_container.hpp"

namespace glaxnimate::model {

class AnimatableBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int keyframe_count READ keyframe_count)
    Q_PROPERTY(bool animated READ animated)

public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Tween,          ///< Value is animated but the given time isn't a keyframe
        IsKeyframe,     ///< Value is animated and the given time is a keyframe
        Mismatch        ///< Value is animated and the current value doesn't match the animated value
    };

    enum class MoveResult
    {
        NotFound,
        OverwrittenDestination,
        Moved,
    };

    enum AnimatablableFlags
    {
        NoFlags = 0,
        HasValue = 1,
        IsWritable = 2,
        IsProperty = 4,
        IsMeta = 8
    };

    struct SetKeyframeInfo
    {
        bool insertion;
    };

    struct MidTransition
    {
        enum Type
        {
            Invalid,
            SingleKeyframe,
            Middle,
        };
        Type type = Invalid;

        QVariant value;
        model::KeyframeTransition from_previous;
        model::KeyframeTransition to_next;
    };

    virtual ~AnimatableBase() = default;


    virtual int animatable_flags() const = 0;

    /**
     * \brief Number of keyframes
     */
    virtual int keyframe_count() const = 0;

    /**
     * \brief Keyframe whose transition contains \p time
     *
     *  This keyframe starts before or at \p time and it ends at \p time
     *
     * If all keyframes are after \p time, returns first one
     * This means keyframe(keyframe_index(t)) is always valid when animated
     *
     *          keyframe_containing
     *          v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     * \return the Corresponding keyframe or nullptr if not found
     */
    Q_INVOKABLE virtual const KeyframeBase* keyframe_containing(FrameTime time) const = 0;
    virtual KeyframeBase* keyframe_containing(FrameTime time) = 0;

    /**
     * \brief Returns the keyframe fully before the given time
     *
     *
     *  keyframe_before
     *  v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     */
    Q_INVOKABLE virtual KeyframeBase* keyframe_before(FrameTime time) = 0;
    virtual const KeyframeBase* keyframe_before(FrameTime time) const = 0;

    /**
     * \brief Returns the keyframe fully before the given time
     *
     *
     *                  keyframe_before
     *                  v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     */
    Q_INVOKABLE virtual KeyframeBase* keyframe_after(FrameTime time) = 0;
    virtual const KeyframeBase* keyframe_after(FrameTime time) const = 0;

    /**
     * \brief Keyframe at that specific time
     * \return the Corresponding keyframe or nullptr if not found
     */
    Q_INVOKABLE virtual const KeyframeBase* keyframe_at(FrameTime time) const = 0;
    virtual KeyframeBase* keyframe_at(FrameTime time) = 0;

    /**
     * \brief Removes all keyframes
     * \post !animated()
     */
    virtual void clear_keyframes() = 0;

    /**
     * \brief Removes the keyframe with the given time
     * \returns whether a keyframe was found and removed
     */
    virtual bool remove_keyframe_at_time(FrameTime time) = 0;

    /**
     * \brief Whether it has multiple keyframes
     */
    bool animated() const
    {
        return keyframe_count() != 0;
    }

    /**
     * If animated(), whether the current value has been changed over the animated value
     */
    Q_INVOKABLE virtual bool value_mismatch() const = 0;

    KeyframeStatus keyframe_status(FrameTime time) const
    {
        if ( !animated() )
            return NotAnimated;
        if ( value_mismatch() )
            return Mismatch;
        if ( keyframe_at(time) )
            return IsKeyframe;
        return Tween;
    }

    Q_INVOKABLE bool has_keyframe(FrameTime time) const
    {
        if ( !animated() )
            return false;
        return keyframe_at(time);
    }

    virtual detail::TypeErasedKeyframeRange keyframe_range() const = 0;
    virtual detail::TypeErasedKeyframeIterator find(model::FrameTime t) const = 0;

    virtual const KeyframeBase* first_keyframe() const = 0;
    virtual const KeyframeBase* last_keyframe() const = 0;

    /**
     * @brief Sets the transition at the given keyframe
     * @param time
     * @param transition
     */
    Q_INVOKABLE virtual void set_transition(FrameTime time, const KeyframeTransition& transition) = 0;

    /**
     * @brief Sets the transition at the keyframe before \p time
     * @param time
     * @param transition
     */
    Q_INVOKABLE virtual void set_transition_before(FrameTime time, const KeyframeTransition& transition) = 0;

    /**
     * \brief Moves a keyframe
     * \param from_time Time of the keyframe to move
     * \param to_time New time for the keyframe
     */
    virtual MoveResult move_keyframe(FrameTime from_time, FrameTime to_time) = 0;

    // Renamed to avoid clashing with BaseProperty
    /**
     * \brief Get the value at the given time
     */
    virtual QVariant value_at_time(FrameTime time) const = 0;
    // Renamed to avoid clashing with BaseProperty
    virtual QVariant static_value() const = 0;
    virtual bool set_static_value(const QVariant& v) = 0;


    /**
     * \brief Sets a value at a keyframe
     * \param time          Time to set the value at
     * \param value         Value to set
     * \param info          If not nullptr, it will be written to with information about what has been node
     * \param force_insert  If \b true, it will always add a new keyframe
     * \post value(time) == \p value && animate() == true
     * \return The keyframe or nullptr if it couldn't be added.
     * If there is already a keyframe at \p time the returned value might be an existing keyframe
     */
    virtual KeyframeBase* set_keyframe(FrameTime time, const QVariant& value, SetKeyframeInfo* info = nullptr, bool force_insert = false) = 0;

    /**
     * \brief Adds a keyframe at the given time
     */
    virtual QUndoCommand* command_add_smooth_keyframe(FrameTime time, const QVariant& value, bool commit = true, QUndoCommand* parent = nullptr);
    virtual QUndoCommand* command_remove_keyframe(FrameTime time, QUndoCommand* parent = nullptr);
    /**
     * \brief Clears all keyframes and creates an associated undo action
     */
    virtual QUndoCommand* command_clear_keyframes(QUndoCommand* parent = nullptr);
    /**
     * @brief Creates an appropriate undo command for changing a keyframe's transition
     */
    virtual QUndoCommand* command_set_transition(model::FrameTime time, const model::KeyframeTransition& transition, QUndoCommand* parent = nullptr);
    virtual QUndoCommand* command_set_transition_side(
        model::FrameTime time,
        model::KeyframeTransition::Descriptive desc,
        const QPointF& point,
        bool before_transition,
        QUndoCommand* parent = nullptr
    );
    virtual QUndoCommand* command_move_keyframe(model::FrameTime time_before, model::FrameTime time_after, QUndoCommand* parent = nullptr);

    // Renamed to avoid clashing with BaseProperty
    virtual QString visual_name() const = 0;

    MidTransition mid_transition(FrameTime time) const;

protected:
    MidTransition do_mid_transition(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const;
    virtual QVariant do_mid_transition_value(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const = 0;


Q_SIGNALS:
    void keyframe_added(FrameTime time);
    void keyframe_removed(FrameTime time);
    void keyframe_updated(FrameTime time);
    void keyframe_moved(FrameTime from_time, FrameTime to_time);
    void transition_changed(FrameTime time, KeyframeTransition::Descriptive before, KeyframeTransition::Descriptive after);

};


namespace detail {

/**
 * @brief Implements common operations based on the keyframe container
 */
template<class KeyframeT, class Base>
class AnimatableImpl : public Base
{
public:
    using Ctor = AnimatableImpl;
    using keyframe_type = KeyframeT;

    using Base::Base;

    int keyframe_count() const override
    {
        return keyframes_.size();
    }

    const keyframe_type* keyframe_containing(FrameTime time) const override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_containing(FrameTime time) override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_before(FrameTime time) override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() || it == keyframes_.begin())
            return nullptr;
        --it;
        return it.ptr();
    }


    keyframe_type* keyframe_after(FrameTime time) override
    {
        auto it = keyframes_.upper_bound(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }


    const keyframe_type* keyframe_before(FrameTime time) const override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() || it == keyframes_.begin())
            return nullptr;
        --it;
        return it.ptr();
    }


    const keyframe_type* keyframe_after(FrameTime time) const override
    {
        auto it = keyframes_.upper_bound(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    const keyframe_type* keyframe_at(FrameTime time) const override
    {
        auto it = keyframes_.find(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_at(FrameTime time) override
    {
        auto it = keyframes_.find(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    detail::TypeErasedKeyframeRange keyframe_range() const override
    {
        return keyframes_.type_erased();
    }

    detail::TypeErasedKeyframeIterator find(model::FrameTime t) const override
    {
        return keyframes_.type_erased(keyframes_.find(t));
    }

    const KeyframeBase* first_keyframe() const override
    {
        return keyframes_.empty() ? nullptr : keyframes_.begin().ptr();
    }

    const KeyframeBase* last_keyframe() const override
    {
        if ( keyframes_.empty() )
            return nullptr;
        auto iter = keyframes_.end();
        --iter;
        return iter.ptr();
    }

    typename KeyframeContainer<KeyframeT>::const_iterator begin() const { return this->keyframes_.begin(); }
    typename KeyframeContainer<KeyframeT>::const_iterator end() const { return this->keyframes_.end(); }

protected:
    KeyframeContainer<KeyframeT> keyframes_;
};

} // namespace detail

} // namespace glaxnimate::model
