/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/math/bezier/bezier.hpp"

namespace glaxnimate::math::bezier {


inline Bezier rect(const QRectF& r)
{
    Bezier bezier;
    bezier.add_point(r.topRight());
    bezier.add_point(r.bottomRight());
    bezier.add_point(r.bottomLeft());
    bezier.add_point(r.topLeft());
    bezier.close();
    return bezier;
}
inline Bezier ellipse(const QPointF& center, const QSizeF& diameter)
{
    QPointF radius{diameter.width() / 2, diameter.height() / 2};
    QPointF tangent = radius * math::ellipse_bezier;
    qreal x = center.x();
    qreal y = center.y();

    Bezier shape;
    shape.close();
    shape.add_point(
        QPointF(x, y - radius.y()),
        QPointF(-tangent.x(), 0),
        QPointF(tangent.x(), 0)
    );
    shape.add_point(
        QPointF(x + radius.x(), y),
        QPointF(0, -tangent.y()),
        QPointF(0, tangent.y())
    );
    shape.add_point(
        QPointF(x, y + radius.y()),
        QPointF(tangent.x(), 0),
        QPointF(-tangent.x(), 0)
    );
    shape.add_point(
        QPointF(x - radius.x(), y),
        QPointF(0, tangent.y()),
        QPointF(0, -tangent.y())
    );
    return shape;
}

inline Bezier ellipse(const QRectF& rect)
{
    return ellipse(rect.center(), rect.size());
}

} // namespace glaxnimate::math::bezier
