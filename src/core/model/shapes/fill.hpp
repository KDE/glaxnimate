/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QBrush>
#include <QPainter>

#include "styler.hpp"
#include "model/animation/animatable.hpp"

namespace glaxnimate::model {

class Fill : public StaticOverrides<Fill, Styler>
{
    GLAXNIMATE_OBJECT(Fill)

public:
    enum Rule
    {
        NonZero = Qt::WindingFill,
        EvenOdd = Qt::OddEvenFill,
    };

private:
    Q_ENUM(Rule);

    GLAXNIMATE_PROPERTY(Rule, fill_rule, NonZero, nullptr, nullptr, PropertyTraits::Visual)

public:
    using Ctor::Ctor;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        return collect_shapes(t, {}).bounding_box();
    }

    static QIcon static_tree_icon()
    {
        return QIcon::fromTheme("format-fill-color");
    }

    static QString static_type_name_human()
    {
        return i18n("Fill");
    }

protected:
    QPainterPath to_painter_path_impl(FrameTime t) const override;

    void on_paint(QPainter* p, FrameTime t, PaintMode, model::Modifier* modifier) const override;
};


} // namespace glaxnimate::model
