/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_gstate.hpp"
#include "glaxnimate/math/ellipse_solver.hpp"
#include "ps_value.hpp"

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

bool glaxnimate::ps::to_float_array(const ValueArray &vals, std::vector<float> &out, bool allow_negative)
{
    out.reserve(vals.size());
    for ( const auto& v : vals )
    {
        if ( v.type() == Value::Real || v.type() == Value::Integer )
        {
            auto d = v.cast<float>();
            if ( allow_negative || d >= 0 )
            {
                out.push_back(d);
                continue;
            }
        }

        return false;
    }

    return true;
}

bool glaxnimate::ps::to_matrix(const ValueArray &vals, QTransform &out)
{
    if ( vals.size() != 6 )
        return false;

    std::vector<float> floats;
    if ( !to_float_array(vals, floats, true) )
        return false;

    out = matrix_from_elements(floats);
    return true;
}

QTransform glaxnimate::ps::to_matrix(const ValueArray &vals)
{
    QTransform tf;
    to_matrix(vals, tf);
    return tf;
}

glaxnimate::ps::ValueArray glaxnimate::ps::matrix_to_array(const QTransform &tf)
{
    ValueArray out;
    matrix_to_array(tf, out);
    return out;

}

void glaxnimate::ps::matrix_to_array(const QTransform &tf, ValueArray &arr)
{
    auto elems = matrix_elements(tf);
    for ( int i = 0; i < int(elems.size()); i++ )
        arr[i] = elems[i];
}
