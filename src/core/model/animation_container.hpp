/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "model/object.hpp"
#include "model/property/property.hpp"

namespace glaxnimate::model {


/**
 * \brief Helper class for document nodes that enclose an animation
 */
class AnimationContainer: public Object
{
    GLAXNIMATE_OBJECT(AnimationContainer)
    GLAXNIMATE_PROPERTY(float, first_frame,  0, &AnimationContainer::on_first_frame_changed, &AnimationContainer::validate_first_frame, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(float, last_frame, -1, &AnimationContainer::on_last_frame_changed,  &AnimationContainer::validate_last_frame,  PropertyTraits::Visual)
    Q_PROPERTY(bool time_visible READ time_visible)
    Q_PROPERTY(float duration READ duration)

public:
    using Object::Object;

    /**
     * \brief Whether time() is within first/last frame
     */
    bool time_visible() const;

    bool time_visible(FrameTime t) const;

    void set_time(FrameTime t) override;

    float duration() const;

    QString type_name_human() const override;

    void stretch_time(qreal multiplier) override;

Q_SIGNALS:
    void first_frame_changed(float);
    void last_frame_changed(float);
    void time_visible_changed(bool visible);

private Q_SLOTS:
    void on_first_frame_changed(float);
    void on_last_frame_changed(float);

private:
    bool validate_first_frame(int f) const;
    bool validate_last_frame(int f) const;
};

} // namespace glaxnimate::model
