/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_gstate.hpp"
#include "glaxnimate/math/ellipse_solver.hpp"

void glaxnimate::ps::GraphicsState::add_arc(const QPointF center, float radius, float angle_start, float angle_end)
{
    add_arc_radians(center, radius, math::deg2rad(angle_start), math::deg2rad(angle_end));
}

void glaxnimate::ps::GraphicsState::add_arc_radians(const QPointF center, float radius, float angle_start, float angle_end)
{
    math::EllipseSolver solver(center, {radius, radius}, 0);
    auto userbez = solver.to_bezier(angle_start, angle_end - angle_start);
    userbez.back().tan_out = userbez.back().pos;
    auto devicebez = userbez.transformed(transform);

    if ( path.empty() )
        path.append(devicebez);
    else
        path.back().append(devicebez);

}
