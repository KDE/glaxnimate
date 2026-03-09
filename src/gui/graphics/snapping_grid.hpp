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
    enum GridShape { Square, Triangle };
    Q_ENUM(GridShape)

    Q_PROPERTY(bool enabled READ is_enabled WRITE enable NOTIFY enabled)
    Q_PROPERTY(unsigned size READ size WRITE set_size NOTIFY size_changed)
    Q_PROPERTY(GridShape shape READ shape WRITE set_shape NOTIFY shape_changed)
    Q_PROPERTY(QPointF origin READ origin WRITE set_origin NOTIFY moved)
    Q_PROPERTY(qreal angle READ angle WRITE set_angle NOTIFY angle_changed)

protected:
    unsigned    m_size = 32;
    GridShape   m_shape = Square;
    QPointF     m_origin = {};
    qreal       m_angle = 0;
    bool        m_enabled = false;
    QTransform  m_global_to_local;
    QTransform  m_local_to_global;

public:
    /// returns closest grid point
    QPointF nearest ( QPointF p ) const { return nearest(p, true); }
    /// returns closest grid point
    QPointF nearest ( double x, double y ) const { return nearest(QPointF(x,y)); }

    /// draws grid lines that cover at least rect
    void render (QPainter *painter, const QPolygonF &rect) const;

    bool is_enabled () const { return m_enabled; }

    double size() const { return m_size; }

    GridShape shape() const { return m_shape; }

    QPointF origin() const { return m_origin; }

    qreal angle() const { return m_angle; }

private:
    void rebuild_tf();
    QPointF nearest ( QPointF p, bool transform ) const;

public Q_SLOTS:
    void enable ( bool enabled );

    void set_size (int size );

    void set_shape ( GridShape shape );

    void set_origin ( QPointF origin );

    void set_angle(qreal angle);

Q_SIGNALS:
    void grid_changed();
    void moved(QPointF);
    void shape_changed(int);
    void enabled(bool);
    void angle_changed(qreal);
    void size_changed(int);
};

} // namespace glaxnimate::gui

#endif // SNAPPING_GRID_HPP

