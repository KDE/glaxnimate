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

    virtual QUndoCommand* add_smooth_keyframe_command(FrameTime time, const QVariant& value);


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

} // namespace glaxnimate::model
