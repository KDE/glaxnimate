/*
 * SPDX-FileCopyrightText: 2012-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "snapping_grid.hpp"
#include <QVarLengthArray>
#include <QLineF>
#include <QApplication>
#include <QPalette>
#include "glaxnimate/math/math.hpp"

static const double sqrt3 = 1.732050808;

using namespace glaxnimate::gui;


QPointF SnappingGrid::nearest(QPointF p, bool transform) const
{
    if ( !m_enabled )
        return p;

    // Transform coordinate system to local
    if ( transform )
        p = m_global_to_local.map(p);

    if ( m_shape == Square )
    {
        /**
            Square grid, simple enough
        */
        p /= m_size;
        p.setX(qRound64(p.x()));
        p.setY(qRound64(p.y()));
        p *= m_size;
    }
    else if ( m_shape == Triangle )
    {
        /**
            Triangular grid, find intersection of line1 and line2
            line2 is horizontal: y = n2 * y_factor + origin.y
            y_factor is the height of the triangle sqrt(3)/2 * size.
            line1 has a 30deg slope, and x offset of n1*size + origin.x
            y = sqrt(3) * ( x + n1*size + origin.x )
            They intersect in the point where y is constrained by line2,
            x = size/2 * n2 + size * n1

            if you think of p as (x,y)+offset, you can get the expressions used
            below to get (x,y) from (p.x,p.y).
            The rounding p/size removes the offset as |offset.x| < size
        */
        double y_factor = m_size * sqrt3/2.0;
        qint64 n2 = qRound64(p.y()/y_factor);
        p.setY(n2*y_factor);
        qint64 n1 = qRound64 ( p.x()/m_size - n2/2.0 );
        p.setX(m_size*(n2/2.0+n1));
    }

    // Transform back to global
    if ( transform )
        p = m_local_to_global.map(p);

    return p;
}

void SnappingGrid::render(QPainter *painter, const QPolygonF &boundary) const
{

    if ( !m_enabled )
        return;

    painter->save();
    painter->setTransform(m_local_to_global, true);
    QRectF rect = m_global_to_local.map(boundary).boundingRect();

    QColor color = qApp->palette().highlight().color();
    color.setAlpha(color.alpha() / 3.);
    painter->setPen(QPen(color, 0));

    /*const QTransform& transform = painter->transform();
    qreal scale = std::sqrt(transform.m11() * transform.m11() + transform.m12() * transform.m12());
    painter->drawEllipse(m_origin, 5 / scale, 5 / scale);*/
    QPointF topleft = nearest(QPointF(rect.left()-m_size, rect.top()-m_size), false);

    QVarLengthArray<QLineF, 128> lines;
    if ( m_shape == SnappingGrid::Square )
    {
        for (double x = topleft.x(); x < rect.right(); x += m_size)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (double y = topleft.y(); y < rect.bottom(); y += m_size)
            lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    else if ( m_shape == SnappingGrid::Triangle )
    {
        double y_factor = m_size*sqrt3/2.0;
        double y;
        for ( y = topleft.y(); y < rect.bottom(); y += y_factor)
            lines.append(QLineF(rect.left(), y, rect.right(), y));

        double slope =  sqrt3;
        double x = topleft.x();
        for ( double x2 = x; x2 < rect.right(); x += m_size )
        {
            x2 = (rect.bottom()-topleft.y())/(-slope)+x;
            lines.append(QLineF(x, topleft.y(),x2,rect.bottom() ));
        }

        for ( double x2 = x; x2 > rect.left(); x -= m_size )
        {
            x2 = (rect.bottom()-topleft.y())/(slope)+x;
            lines.append(QLineF(x, topleft.y(),x2,rect.bottom() ));
        }

    }

    painter->drawLines(lines.data(), lines.size());
    painter->restore();
}

void SnappingGrid::enable(bool enable)
{
    m_enabled = enable;
    Q_EMIT enabled(enable);
    Q_EMIT grid_changed();
}

void SnappingGrid::set_size(int size)
{
    if ( size > 0 )
    {
        m_size = size;
        Q_EMIT size_changed(size);
        Q_EMIT grid_changed();
    }
}

void SnappingGrid::set_shape(SnappingGrid::GridShape shape)
{
    m_shape = shape;
    Q_EMIT shape_changed(int(shape));
    Q_EMIT grid_changed();
}

void SnappingGrid::set_origin(QPointF origin)
{
    m_origin = origin;
    rebuild_tf();
    Q_EMIT moved(origin);
    Q_EMIT grid_changed();
}


void glaxnimate::gui::SnappingGrid::set_angle(qreal angle)
{
    m_angle = angle;
    rebuild_tf();
    Q_EMIT angle_changed(angle);
    Q_EMIT grid_changed();
}

void glaxnimate::gui::SnappingGrid::rebuild_tf()
{
    m_local_to_global = QTransform();
    m_local_to_global.rotateRadians(m_angle);
    m_local_to_global.translate(m_origin.x(), m_origin.y());

    m_global_to_local = QTransform();
    m_global_to_local.translate(-m_origin.x(), -m_origin.y());
    m_global_to_local.rotateRadians(-m_angle);
}
