/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "keyframe_transition.hpp"
#include "math/bezier/segment.hpp"
#include "math/polynomial.hpp"

namespace {

constexpr QPointF bound_vec(const QPointF& v)
{
    return {
        qBound(glaxnimate::math::scalar_type<QPointF>(0), v.x(), glaxnimate::math::scalar_type<QPointF>(1)),
        v.y()
    };
}

} // namespace

glaxnimate::model::KeyframeTransition::Descriptive glaxnimate::model::KeyframeTransition::before_descriptive() const
{
    if ( hold_ )
        return Hold;

    if ( qFuzzyIsNull(bezier_.points()[1].x() - bezier_.points()[1].y()) )
        return Linear;

    if ( bezier_.points()[1].y() == 0 )
        return Ease;

    if ( bezier_.points()[1].y() < 0 )
        return Overshoot;

    if ( bezier_.points()[1].x() < bezier_.points()[1].y() )
        return Fast;

    return Custom;
}

glaxnimate::model::KeyframeTransition::Descriptive glaxnimate::model::KeyframeTransition::after_descriptive() const
{
    if ( hold_ )
        return Hold;

    if ( qFuzzyIsNull(bezier_.points()[2].x() - bezier_.points()[2].y()) )
        return Linear;

    if ( bezier_.points()[2].y() == 1 )
        return Ease;

    if ( bezier_.points()[2].y() > 1 )
        return Overshoot;

    if ( bezier_.points()[2].x() > bezier_.points()[2].y() )
        return Fast;

    return Custom;
}

void glaxnimate::model::KeyframeTransition::set_before_descriptive(model::KeyframeTransition::Descriptive d)
{
    switch ( d )
    {
        case Hold:
            set_hold(true);
            return;
        case Linear:
            bezier_.set<1>(QPointF{1./3., 1./3.});
            hold_ = false;
            break;
        case Ease:
            bezier_.set<1>(QPointF{1./3., 0});
            hold_ = false;
            break;
        case Fast:
            bezier_.set<1>(QPointF{1./6., 1./3.});
            hold_ = false;
            break;
        case Overshoot:
            bezier_.set<1>(QPointF{2./3., -1./3.});
            hold_ = false;
            break;
        case Custom:
            hold_ = false;
            break;
    }
}

void glaxnimate::model::KeyframeTransition::set_after_descriptive(model::KeyframeTransition::Descriptive d)
{
    switch ( d )
    {
        case Hold:
            set_hold(true);
            return;
        case Linear:
            bezier_.set<2>(QPointF{2./3., 2./3.});
            hold_ = false;
            break;
        case Ease:
            bezier_.set<2>(QPointF{2./3., 1});
            hold_ = false;
            break;
        case Fast:
            bezier_.set<2>(QPointF{5./6., 2./3.});
            hold_ = false;
            break;
        case Overshoot:
            bezier_.set<2>(QPointF{1./3., 4./3.});
            hold_ = false;
            break;
        case Custom:
            hold_ = false;
            break;
    }
}

void glaxnimate::model::KeyframeTransition::set_after(const QPointF& after)
{
    bezier_.set<2>(bound_vec(after));
}

void glaxnimate::model::KeyframeTransition::set_before(const QPointF& before)
{
    bezier_.set<1>(bound_vec(before));
}

void glaxnimate::model::KeyframeTransition::set_handles(const QPointF& before, const QPointF& after)
{
    set_before(before);
    set_after(after);
}

void glaxnimate::model::KeyframeTransition::set_hold(bool hold)
{
    hold_ = hold;
}

double glaxnimate::model::KeyframeTransition::lerp_factor(double ratio) const
{
    if ( hold_ )
    {
        if ( ratio >= 1 || qFuzzyCompare(float(ratio), 1.f) )
            return 1;
        return 0;
    }

    if ( ratio <= 0 )
        return 0;
    if ( ratio >= 1 )
        return 1;
    double t = bezier_.t_at_value(ratio);
    return bezier_.solve_component(t, 1);
}

double glaxnimate::model::KeyframeTransition::bezier_parameter(double ratio) const
{
    if ( ratio <= 0 || hold_ )
        return 0;
    if ( ratio >= 1 )
        return 1;
    return bezier_.t_at_value(ratio);
}

glaxnimate::model::KeyframeTransition::KeyframeTransition(const QPointF& before_handle, const QPointF& after_handle, bool hold)
    : bezier_({0, 0}, before_handle, after_handle, {1,1}),
    hold_(hold)
{}

glaxnimate::model::KeyframeTransition::KeyframeTransition(
    glaxnimate::model::KeyframeTransition::Descriptive before,
    glaxnimate::model::KeyframeTransition::Descriptive after)
    : KeyframeTransition()
{
    set_before_descriptive(before);
    set_after_descriptive(after);
}

glaxnimate::model::KeyframeTransition::KeyframeTransition(glaxnimate::model::KeyframeTransition::Descriptive descriptive)
    : KeyframeTransition(descriptive, descriptive)
{
}

std::pair<glaxnimate::model::KeyframeTransition, glaxnimate::model::KeyframeTransition> glaxnimate::model::KeyframeTransition::split(double x) const
{
    return split_t(bezier_.t_at_value(x));
}

std::pair<glaxnimate::model::KeyframeTransition, glaxnimate::model::KeyframeTransition> glaxnimate::model::KeyframeTransition::split_t(double t) const
{
    if ( hold_ )
        return { {{0, 0}, {1, 1}, true}, {{0, 0}, {1, 1}, true} };

    if ( qFuzzyIsNull(t) )
    {
        return { {{0, 0}, {1, 1}, false}, *this };
    }
    else if ( qFuzzyCompare(t, 1) )
    {
        return { *this, {{0, 0}, {1, 1}, false} };
    }

    qreal x = bezier_.solve_component(t, 0);
    qreal y = bezier_.solve_component(t, 1);
    math::bezier::BezierSegment left, right;
    std::tie(left, right) = bezier_.split(t);

    qreal left_factor_x = 1 / x;
    qreal left_factor_y = 1 / y;
    qreal right_factor_x = 1 / (1-x);
    qreal right_factor_y = 1 / (1-y);
    qreal right_offset_y = 0;

    QPointF left_p1{left[1].x() * left_factor_x, left[1].y() * left_factor_y};
    QPointF left_p2{left[2].x() * left_factor_x, left[2].y() * left_factor_y};
    QPointF right_p1{(right[1].x() - x) / (1-x), (right[1].y() - y) / (1-y)};
    QPointF right_p2{(right[2].x() - x) / (1-x), (right[2].y() - y) / (1-y)};

    if ( y < 0 )
    {
        left_p1.setY(-left[1].y() / (y - 1));
        left_p2.setY(1 - left[2].y() / (y - 1));
    }
    else if ( y > 1 )
    {
        right_p1.setY(-(right[1].y() - 1) / (y - 1));
        right_p2.setY(1 - (right[2].y() - 1) / (y - 1));
    }

    return {
        {
            {left[1].x() * left_factor_x, left[1].y() * left_factor_y},
            {left[2].x() * left_factor_x, left[2].y() * left_factor_y}
        },
        {
            {(right[1].x() - x) * right_factor_x, right_offset_y + (right[1].y() - y) * right_factor_y},
            {(right[2].x() - x) * right_factor_x, right_offset_y + (right[2].y() - y) * right_factor_y}
        }
    };
}
