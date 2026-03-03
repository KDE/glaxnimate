/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/mask_settings.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::MaskSettings)

QString glaxnimate::model::MaskSettings::type_name_human() const
{
    return i18n("Mask");
}

glaxnimate::model::MaskSettings::MaskMode glaxnimate::model::MaskSettings::next_mode(MaskMode previous)
{
    switch ( previous )
    {
        case NoMask:
            return Alpha;
        case Alpha:
            return Luma;
        case Luma:
        default:
            return NoMask;
    }
}
