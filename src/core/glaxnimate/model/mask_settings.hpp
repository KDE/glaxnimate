/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/object.hpp"
#include "glaxnimate/model/property/reference_property.hpp"
#include "glaxnimate/model/animation/frame_time.hpp"
#include "glaxnimate/model/shapes/shape.hpp"

namespace glaxnimate::model {

class MaskSettings : public Object
{
    GLAXNIMATE_OBJECT(MaskSettings)

public:
    enum MaskMode
    {
        NoMask = 0,
        Alpha = renderer::MaskSourceAlpha,
        Luma = renderer::MaskSourceLuma,
    };
    Q_ENUM(MaskMode)

    GLAXNIMATE_PROPERTY(MaskMode, mask, NoMask, &MaskSettings::mask_changed, {}, PropertyTraits::Visual)
    GLAXNIMATE_PROPERTY(bool, inverted, false, &MaskSettings::inverted_changed, {}, PropertyTraits::Visual)

public:
    using Object::Object;

    QString type_name_human() const override;

    bool has_mask() const { return mask.get(); }

    /**
     * \brief Returns all modes in a cycle
     */
    static MaskMode next_mode(MaskMode previous);

Q_SIGNALS:
    void mask_changed();
    void inverted_changed();

};


} // namespace glaxnimate::model
