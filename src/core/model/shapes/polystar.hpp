#pragma once

#include "shape.hpp"

namespace model {


class PolyStar : public ObjectBase<PolyStar, Shape>
{
    GLAXNIMATE_OBJECT

public:
    enum StarType
    {
        Star = 1,
        Polygon = 2,
    };

    Q_ENUM(StarType)

    GLAXNIMATE_PROPERTY(StarType, type, Star, {}, {}, PropertyTraits::Visual)
    GLAXNIMATE_ANIMATABLE(QPointF, position, QPointF())
//     GLAXNIMATE_ANIMATABLE(float, inner_roundness, 0)
    GLAXNIMATE_ANIMATABLE(float, outer_radius, 0)
    GLAXNIMATE_ANIMATABLE(float, inner_radius, 0)
//     GLAXNIMATE_ANIMATABLE(float, outner_roundness, 0)
    GLAXNIMATE_ANIMATABLE(float, angle, 0)
    GLAXNIMATE_ANIMATABLE(int, points, 5)

public:
    using Ctor::Ctor;

    QIcon docnode_icon() const override
    {
        if ( type.get() == Star )
            return QIcon::fromTheme("draw-star");
        return QIcon::fromTheme("draw-polygon");
    }

    QString type_name_human() const override
    {
        return tr("PolyStar");
    }

    math::Bezier to_bezier(FrameTime t) const override;

    QRectF local_bounding_rect(FrameTime t) const override
    {
        float radius = qMax(this->outer_radius.get_at(t), this->inner_radius.get_at(t));
        return QRectF(position.get_at(t) - QPointF(radius, radius), QSizeF(radius*2, radius*2));
    }

    static math::Bezier draw(StarType type, const QPointF& pos, float r1, float r2, float angle_radians, int p);
};

} // namespace model

