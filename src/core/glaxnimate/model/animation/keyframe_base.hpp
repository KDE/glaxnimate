/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <QVariant>

#include "glaxnimate/model/animation/keyframe_transition.hpp"
#include "glaxnimate/model/animation/frame_time.hpp"

namespace glaxnimate::model {

class KeyframeBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(double time READ time)

public:
    explicit KeyframeBase(FrameTime time) : time_ { time } {}
    virtual ~KeyframeBase() = default;
    KeyframeBase(KeyframeBase&& oth) :
        time_(oth.time_)
    {}

    virtual QVariant value() const = 0;
    virtual bool set_value(const QVariant& value) = 0;

    FrameTime time() const { return time_; }
    void set_time(FrameTime t) { time_ = t; }

    /**
     * \brief Transition into the next value
     */
    virtual KeyframeTransition transition() const = 0;
    virtual void set_transition(const KeyframeTransition& trans) = 0;

    void stretch_time(qreal multiplier)
    {
        time_ *= multiplier;
    }

    /**
     * \brief Splits a keyframe into multiple segments
     * \param other The keyframe following this
     * \param splits Array of splits in [0, 1], indicating the fractions at which splits shall occur
     * \pre \p other must be the same type of keyframe as \b this
     * \returns An array of keyframes matching the splits,
     *      this will include a copy of \p other, which might have been modified slightly.
     *      This should be used as \p this for the next keyframe
     */
    std::vector<std::unique_ptr<KeyframeBase>> split(const KeyframeBase* other, std::vector<qreal> splits) const;

    std::unique_ptr<KeyframeBase> clone() const
    {
        auto clone = do_clone();
        return clone;
    }

protected:
    virtual std::unique_ptr<KeyframeBase> do_clone() const = 0;

    class KeyframeSplitter
    {
    public:
        virtual ~KeyframeSplitter() = default;
        virtual void step(const QPointF& p) = 0;
        virtual std::unique_ptr<KeyframeBase> left(const QPointF& p) const = 0;
        virtual std::unique_ptr<KeyframeBase> right(const QPointF& p) const = 0;
        virtual std::unique_ptr<KeyframeBase> last() const = 0;
    };

    virtual std::unique_ptr<KeyframeSplitter> splitter(const KeyframeBase* other) const = 0;

private:
    FrameTime time_;
};


} // namespace glaxnimate::model
