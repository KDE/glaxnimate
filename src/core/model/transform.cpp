/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "transform.hpp"
#include "math/math.hpp"

namespace {

QTransform make_transform(
    const QPointF& anchor_point,
    const QPointF& position,
    double rotation,
    QVector2D scale,
    const std::optional<QPointF>& pos_derivative
)
{
    QTransform trans;
    trans.translate(position.x(), position.y());
    trans.rotate(rotation);
    trans.scale(scale.x(), scale.y());
    trans.translate(-anchor_point.x(), -anchor_point.y());
    if ( pos_derivative )
        trans.rotate(glaxnimate::math::rad2deg(glaxnimate::math::atan2(pos_derivative->y(), pos_derivative->x())));
    return trans;
}

} // namespace

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::Transform)

QTransform glaxnimate::model::Transform::transform_matrix(FrameTime f, bool auto_orient) const
{
    std::optional<QPointF> pos_derivative;
    if ( auto_orient )
        pos_derivative = position.derivative_at(f);

    return make_transform(
        anchor_point.get_at(f),
        position.get_at(f),
        rotation.get_at(f),
        scale.get_at(f),
        pos_derivative
    );
}

void glaxnimate::model::Transform::set_transform_matrix(const QTransform& t)
{
    qreal a = t.m11();
    qreal b = t.m12();
    qreal c = t.m21();
    qreal d = t.m22();
    qreal tx = t.m31();
    qreal ty = t.m32();

    position.set(QPointF(tx, ty));
    qreal delta = a * d - b * c;
    qreal sx = 1;
    qreal sy = 1;
    if ( a != 0 || b != 0 )
    {
        qreal r = math::hypot(a, b);
        rotation.set(-math::rad2deg(-math::sign(b) * math::acos(a/r)));
        sx = r;
        sy = delta / r;
    }
    else
    {
        qreal r = math::hypot(c, d);
        rotation.set(-math::rad2deg(math::pi / 2 + math::sign(d) * math::acos(c / r)));
        sx = delta / r;
        sy = r;
    }

    scale.set(QVector2D(sx, sy));
}

void glaxnimate::model::Transform::copy(glaxnimate::model::Transform* other)
{
    other->clone_into(this);
}
