/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QIcon>

#include "glaxnimate_app.hpp"
#include "model/animation/keyframe_transition.hpp"

namespace glaxnimate::gui {

struct KeyframeTransitionData
{
    static constexpr const int count = model::KeyframeTransition::Custom + 1;

    enum Side
    {
        Full,
        Start,
        Finish,
    };

    QString name;
    Side side;
    const char* icon_slug;
    model::KeyframeTransition::Descriptive value;

    QIcon icon(Side side) const
    {
        const char* side_path = "";
        if ( side == Start )
            side_path = "/start";
        else if ( side == Finish )
            side_path = "/finish";

        return QIcon(GlaxnimateApp::instance()->data_file(QString("images/keyframe%1/%2.svg").arg(side_path).arg(icon_slug)));
    }

    QIcon icon() const
    {
        return icon(side);
    }

    QVariant variant() const
    {
        return QVariant::fromValue(value);
    }

    static KeyframeTransitionData data(model::KeyframeTransition::Descriptive value, Side side = Full)
    {
        switch ( value )
        {
            case model::KeyframeTransition::Hold:
                return {i18n("Hold"), side, "hold", value};
            case model::KeyframeTransition::Linear:
                return {i18n("Linear"), side, "linear", value};
            case model::KeyframeTransition::Ease:
                return {ease_name(side), side, "ease", value};
            case model::KeyframeTransition::Fast:
                return {i18n("Fast"), side, "fast", value};
            case model::KeyframeTransition::Overshoot:
                return {overshoot_name(side), side, "overshoot", value};
            default:
            case model::KeyframeTransition::Custom:
                return {i18n("Custom"), side, "custom", value};
        }
    }

    static KeyframeTransitionData from_index(int index, Side side = Full)
    {
        return data(model::KeyframeTransition::Descriptive(index), side);
    }

private:
    static QString ease_name(Side side)
    {
        switch ( side )
        {
            case Start:
                return i18n("Ease In");
            case Finish:
                return i18n("Ease Out");
            default:
            case Full:
                return i18n("Ease");
        }
    }

    static QString overshoot_name(Side side)
    {
        switch ( side )
        {
            case Start:
                return i18n("Anticipate");
            case Finish:
                return i18n("Overshoot");
            default:
            case Full:
                return i18n("Overshoot");
        }
    }
};

} // namespace glaxnimate::gui
