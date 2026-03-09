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

static const double sqrt3 = 1.732050808;

using namespace glaxnimate::gui;

SnappingGrid::SnappingGrid(unsigned size, SnappingGrid::GridShape shape,
                             QPointF origin, bool enabled)
    : m_size(size > 0 ? size : 1), m_shape(shape),
      m_origin(origin), m_enabled(enabled)
{
}

void SnappingGrid::snap(QPointF &p) const
{
    if ( !m_enabled )
        return;

    if ( m_shape == Square )
    {
        /**
            Square grid, simple enough
        */
        p -= m_origin;
        p /= m_size;
        p.setX(qRound64(p.x()));
        p.setY(qRound64(p.y()));
        p *= m_size;
        p += m_origin;
    }
    else if ( m_shape == Triangle1 )
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

            the p -= origin, p += origin is to transform the coordinates
            relative to the grid origin
        */
        p -= m_origin;
        double y_factor = m_size * sqrt3/2.0;
        qint64 n2 = qRound64(p.y()/y_factor);
        p.setY(n2*y_factor);
        qint64 n1 = qRound64 ( p.x()/m_size - n2/2.0 );
        p.setX(m_size*(n2/2.0+n1));
        p+=m_origin;
    }
    else if ( m_shape == Triangle2 )
    {
        p -= m_origin;
        double x_factor = m_size * sqrt3/2.0;
        qint64 n2 = qRound64(p.x()/x_factor);
        p.setX(n2*x_factor);
        qint64 n1 = qRound64 ( p.y()/m_size - n2/2.0 );
        p.setY(m_size*(n2/2.0+n1));
        p+=m_origin;
    }
}

void SnappingGrid::render(QPainter *painter, const QRectF &rect) const
{

    if ( !m_enabled )
        return;

    QColor color = qApp->palette().highlight().color();
    color.setAlpha(color.alpha() / 2.);
    painter->setPen(QPen(color, 0));

    /*const QTransform& transform = painter->transform();
    qreal scale = std::sqrt(transform.m11() * transform.m11() + transform.m12() * transform.m12());
    painter->drawEllipse(m_origin, 5 / scale, 5 / scale);*/
    QPointF topleft = nearest(rect.left()-m_size,rect.top()-m_size);

    QVarLengthArray<QLineF, 128> lines;
    if ( m_shape == SnappingGrid::Square )
    {
        for (double x = topleft.x(); x < rect.right(); x += m_size)
            lines.append(QLineF(x, rect.top(), x, rect.bottom()));
        for (double y = topleft.y(); y < rect.bottom(); y += m_size)
            lines.append(QLineF(rect.left(), y, rect.right(), y));
    }
    else if ( m_shape == SnappingGrid::Triangle1 )
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
    else if ( m_shape == SnappingGrid::Triangle2 )
    {
        double x_factor = m_size*sqrt3/2.0;
        double x;
        for ( x = topleft.x(); x < rect.right(); x += x_factor)
                lines.append(QLineF(x,rect.top(), x, rect.bottom()));

        double slope =  sqrt3;
        double y = topleft.y();
        for ( double y2 = y; y2 < rect.bottom(); y += m_size )
        {
            y2 = (rect.right()-topleft.x())/(-slope)+y;
            lines.append(QLineF(topleft.x(),y,rect.right(),y2 ));
        }

        for ( double y2 = y; y2 > rect.top(); y -= m_size )
        {
            y2 = (rect.right()-topleft.x())/(slope)+y;
            lines.append(QLineF(topleft.x(),y,rect.right(),y2 ));
        }

    }

    painter->drawLines(lines.data(), lines.size());
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
    Q_EMIT grid_changed();
    Q_EMIT moved(origin);
}


