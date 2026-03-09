/*
 * SPDX-FileCopyrightText: 2012-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef SNAPPING_GRID_HPP
#define SNAPPING_GRID_HPP

#include <QObject>
#include <QPainter>

namespace glaxnimate::gui {

class SnappingGrid : public QObject
{
    Q_OBJECT

public:
    enum GridShape { Square, Triangle1, Triangle2 };
    Q_ENUM(GridShape)

protected:
    unsigned    m_size;
    GridShape   m_shape;
    QPointF     m_origin;
    bool        m_enabled;

public:
    explicit SnappingGrid ( unsigned size = 32,
                             GridShape shape = Square,
                             QPointF origin=QPointF(0,0),
                             bool enabled = false );

    /// move p to closest grid point
    void snap ( QPointF &p ) const;
    /// returns closest grid point
    QPointF nearest ( QPointF p ) const { snap(p); return p; }
    /// returns closest grid point
    QPointF nearest ( double x, double y ) const { return nearest(QPointF(x,y)); }

    /// draws grid lines that cover at least rect
    void render (QPainter *painter, const QRectF &rect) const;



    bool is_enabled () const { return m_enabled; }

    double size() const { return m_size; }

    GridShape shape() const { return m_shape; }

    QPointF origin() const { return m_origin; }


public Q_SLOTS:

    void enable ( bool enabled );

    void set_size (int size );

    void set_shape ( GridShape shape );

    void set_origin ( QPointF origin );

Q_SIGNALS:
    void grid_changed();
    void moved(QPointF);
    void shape_changed(int);
    void enabled(bool);
};

} // namespace glaxnimate::gui

#endif // SNAPPING_GRID_HPP

