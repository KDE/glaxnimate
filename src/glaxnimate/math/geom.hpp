#pragma once

#include <QPointF>
#include "vector.hpp"

namespace glaxnimate::math {

/**
 * \brief Finds the closest point to a line
 * \param line_a Point to determine the line
 * \param line_b Second point to determine the line
 * \param p      Point to find the closest of
 * \returns The point on the line that is closest to \p p
 */
QPointF line_closest_point(const QPointF& line_a, const QPointF& line_b, const QPointF& p);

/**
 * \brief Gets the center of the circle passing through 3 points
 */
QPointF circle_center(const QPointF& p1, const QPointF& p2, const QPointF& p3);

} // namespace glaxnimate::math
