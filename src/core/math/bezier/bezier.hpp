/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <set>
#include <QPointF>
#include <QPainterPath>
#include "math/bezier/solver.hpp"
#include "math/bezier/point.hpp"
#include "math/bezier/segment.hpp"
#include "utils/value_cache.hpp"

namespace glaxnimate::math::bezier {

using Solver = math::bezier::CubicBezierSolver<QPointF>;

class Bezier
{
public:
    using value_type = Point;

    Bezier() = default;
    explicit Bezier(const Point& initial_point)
        : points_(1, initial_point)
    {}
    explicit Bezier(const QPointF& initial_point)
        : points_(1, initial_point)
    {}

    const std::vector<Point>& points() const { return points_; }
    std::vector<Point>& points() { return points_; }

    int size() const { return points_.size(); }
    int closed_size() const { return points_.size() + (closed_ ? 1 : 0); }
    bool empty() const { return points_.empty(); }
    auto begin() { cache.mark_dirty(); return points_.begin(); }
    auto begin() const { return points_.begin(); }
    auto cbegin() const { return points_.begin(); }
    auto end() { cache.mark_dirty(); return points_.end(); }
    auto end() const { return points_.end(); }
    auto cend() const { return points_.end(); }
    void push_back(const Point& p) { cache.mark_dirty(); points_.push_back(p); }
    void clear() { cache.mark_dirty(); points_.clear(); closed_ = false; }
    const Point& back() const { return points_.back(); }
    Point& back() { cache.mark_dirty(); return points_.back(); }

    const Point& operator[](int index) const { return points_[index % points_.size()]; }
    Point& operator[](int index) { cache.mark_dirty(); return points_[index % points_.size()]; }

    bool closed() const { return closed_; }
    void set_closed(bool closed) { cache.mark_dirty(); closed_ = closed; }

    /**
     * \brief Inserts a point at the given index
     * \param index Index to insert the point at
     * \param p     Point to add
     * \returns \c this, for easy chaining
     */
    Bezier& insert_point(int index, const Point& p)
    {
        cache.mark_dirty();
        points_.insert(points_.begin() + qBound(0, index, size()), p);
        return *this;
    }

    /**
     * \brief Appends a point to the curve (relative tangents)
     * \see insert_point()
     */
    Bezier& add_point(const QPointF& p, const QPointF& in_t = {0, 0}, const QPointF& out_t = {0, 0})
    {
        cache.mark_dirty();
        points_.push_back(Point::from_relative(p, in_t, out_t));
        return *this;
    }

    /**
     * \brief Appends a point with symmetrical (relative) tangents
     * \see insert_point()
     */
    Bezier& add_smooth_point(const QPointF& p, const QPointF& in_t)
    {
        cache.mark_dirty();
        points_.push_back(Point::from_relative(p, in_t, -in_t, Smooth));
        return *this;
    }

    /**
     * \brief Closes the bezier curve
     * \returns \c this, for easy chaining
     */
    Bezier& close()
    {
        cache.mark_dirty();
        closed_ = true;
        return *this;
    }

    /**
     * \brief Line from the last point to \p p
     * \returns \c this, for easy chaining
     */
    Bezier& line_to(const QPointF& p)
    {
        cache.mark_dirty();
        if ( !empty() )
            points_.back().tan_out = points_.back().pos;
        points_.push_back(p);
        return *this;
    }

    /**
     * \brief Quadratic bezier from the last point to \p dest
     * \param handle Quadratic bezier handle
     * \param dest   Destination point
     * \returns \c this, for easy chaining
     */
    Bezier& quadratic_to(const QPointF& handle, const QPointF& dest)
    {
        cache.mark_dirty();
        if ( !empty() )
            points_.back().tan_out = points_.back().pos + 2.0/3.0 * (handle - points_.back().pos);

        push_back(dest);
        points_.back().tan_in = points_.back().pos + 2.0/3.0 * (handle - points_.back().pos);

        return *this;
    }

    /**
     * \brief Cubic bezier from the last point to \p dest
     * \param handle1   First cubic bezier handle
     * \param handle2   Second cubic bezier handle
     * \param dest      Destination point
     * \returns \c this, for easy chaining
     */
    Bezier& cubic_to(const QPointF& handle1, const QPointF& handle2, const QPointF& dest)
    {
        cache.mark_dirty();
        if ( !empty() )
            points_.back().tan_out = handle1;

        push_back(dest);
        points_.back().tan_in = handle2;

        return *this;
    }

    /**
     * \brief Reverses the orders of the points
     */
    void reverse();

    QRectF bounding_box() const;

    /**
     * \brief Split a segmet
     * \param index index of the point at the beginning of the segment to split
     * \param factor Value between [0,1] to determine the split point
     * \post size() increased by one and points[index+1] is the new point
     */
    void split_segment(int index, qreal factor);

    /**
     * \brief The point you'd get by calling split_segment(index, factor)
     */
    Point split_segment_point(int index, qreal factor) const;

    void remove_point(int index)
    {
        cache.mark_dirty();
        if ( index >= 0 && index < size() )
            points_.erase(points_.begin() + index);
    }

    void add_to_painter_path(QPainterPath& out) const;

    math::bezier::Bezier lerp(const math::bezier::Bezier& other, qreal factor) const;

    void set_point(int index, const math::bezier::Point& p)
    {
        cache.mark_dirty();
        if ( index >= 0 && index < size() )
            points_[index] = p;
    }

    BezierSegment segment(int index) const;
    void set_segment(int index, const BezierSegment& s);
    BezierSegment inverted_segment(int index) const;

    int segment_count() const;

    Bezier transformed(const QTransform& t) const;
    void transform(const QTransform& t);

    /**
     * \brief Returns a new bezier with the given points removed
     */
    math::bezier::Bezier removed_points(const std::set<int>& indices) const;

    /**
     * \brief For closed beziers, ensure the last segment is present
     */
    void add_close_point();

    /**
     * \brief Sets the given point to the type, and adjusts its tangents as needed
     * \returns The updated point
     * \note This doesn't update the bezier itself
     */
    Point point_with_type(int index, math::bezier::PointType point_type) const;

private:
    /**
     * \brief Solver for the point \p p to the point \p p + 1
     */
    math::bezier::CubicBezierSolver<QPointF> solver_for_point(int p) const
    {
        return segment(p);
    }

    std::vector<Point> points_;
    bool closed_ = false;
    mutable util::ValueCache<QRectF> cache;
};


class MultiBezier
{
public:
    MultiBezier() {}
    MultiBezier(const QPainterPath& path) { append(path); }
    const std::vector<Bezier>& beziers() const { return beziers_; }
    std::vector<Bezier>& beziers() { return beziers_; }

    Bezier& back() { cache.mark_dirty(); return beziers_.back(); }
    const Bezier& back() const { return beziers_.back(); }

    MultiBezier& move_to(const QPointF& p);

    MultiBezier& line_to(const QPointF& p);

    MultiBezier& quadratic_to(const QPointF& handle, const QPointF& dest);

    MultiBezier& cubic_to(const QPointF& handle1, const QPointF& handle2, const QPointF& dest);

    MultiBezier& close();

    QRectF bounding_box() const;

    QPainterPath painter_path() const;

    void append(const MultiBezier& other);

    void append(const QPainterPath& path);

    void append(const QPolygonF& path);

    void append(const Bezier& path);

    template<class T>
    MultiBezier& operator+= (const T& val)
    {
        append(val);
        return *this;
    }

    static MultiBezier from_painter_path(const QPainterPath& path);

    int size() const { return beziers_.size(); }
    bool empty() const { return beziers_.empty(); }
    void clear() { cache.mark_dirty(); beziers_.clear(); }

    auto begin() { cache.mark_dirty(); return beziers_.begin(); }
    auto begin() const { return beziers_.begin(); }
    auto cbegin() const { return beziers_.begin(); }
    auto end() { cache.mark_dirty(); return beziers_.end(); }
    auto end() const { return beziers_.end(); }
    auto cend() const { return beziers_.end(); }

    const Bezier& operator[](int index) const
    {
        return beziers_[index];
    }

    Bezier& operator[](int index)
    {
        cache.mark_dirty();
        return beziers_[index];
    }

    void transform(const QTransform& t);
    glaxnimate::math::bezier::MultiBezier transformed(const QTransform& matrix) const;

    void translate(const QPointF& p);
    glaxnimate::math::bezier::MultiBezier translated(const QPointF& p) const;

    void reverse();
    glaxnimate::math::bezier::MultiBezier reversed() const;

private:
    void handle_end()
    {
        if ( at_end )
        {
            beziers_.push_back(Bezier());
            if ( beziers_.size() > 1 )
                beziers_.back().add_point(beziers_[beziers_.size()-2].points().back().pos);
            at_end = false;
        }
    }

    std::vector<Bezier> beziers_;
    bool at_end = true;
    mutable util::ValueCache<QRectF> cache;
};

} // namespace glaxnimate::math

namespace glaxnimate::math {

inline bezier::Bezier lerp(const math::bezier::Bezier& a, const math::bezier::Bezier& b, qreal factor)
{
    return a.lerp(b, factor);
}

} // namespace glaxnimate::math

Q_DECLARE_METATYPE(glaxnimate::math::bezier::Bezier)

QDataStream &operator<<(QDataStream &out, const glaxnimate::math::bezier::Bezier& val);

QDataStream &operator>>(QDataStream &in, glaxnimate::math::bezier::Bezier &val);
