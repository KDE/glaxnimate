#pragma once

#include"glaxnimate/math/bezier/solver.hpp"

#include <QObject>
#include <QPointF>


namespace glaxnimate::model {

namespace detail {
    using SampleCache = std::vector<double>;
} // namespace detail

/**
 * \brief Describes the easing between two keyframes
 */
class KeyframeTransition
{
    Q_GADGET

public:
    enum Descriptive
    {
        Hold,
        Linear,
        Ease,
        Custom,
    };

    Q_ENUM(Descriptive)

    KeyframeTransition() = default;
    KeyframeTransition(const QPointF& before_handle, const QPointF& after_handle, bool hold = false);

    const math::bezier::CubicBezierSolver<QPointF>& bezier() const { return bezier_; }
    bool hold() const { return hold_; }

    Descriptive before_descriptive() const;
    Descriptive after_descriptive() const;
    QPointF before() const { return bezier_.points()[1]; }
    QPointF after() const  { return bezier_.points()[2]; }

    /**
     * \brief Get interpolation factor
     * \param ratio in [0, 1]. Determines the time ratio (0 = before, 1 = after)
     * \return A value in [0, 1]: the corresponding interpolation factor
     *
     * If the bezier is defined as B(t) = (x,y). This gives y given x.
     */
    double lerp_factor(double ratio) const;

    /**
     * \brief Get the bezier parameter at the given time
     * \param ratio in [0, 1]. Determines the time ratio (0 = before, 1 = after)
     * \return A value in [0, 1]: the corresponding bezier parameter
     *
     * If the bezier is defined as B(t) = (x,y). This gives t given x.
     */
    double bezier_parameter(double ratio) const;

    void set_hold(bool hold);
    void set_before_descriptive(Descriptive d);
    void set_after_descriptive(Descriptive d);
    void set_handles(const QPointF& before, const QPointF& after);
    void set_before(const QPointF& before);
    void set_after(const QPointF& after);

    /**
     * \brief Split the transition at \p x
     * \return The transitions before/after the split
     */
    std::pair<KeyframeTransition, KeyframeTransition> split(double x) const;

private:
    math::bezier::CubicBezierSolver<QPointF> bezier_ { QPointF(0, 0), QPointF(0, 0), QPointF(1, 1), QPointF(1, 1) };
    bool hold_ = false;
    mutable detail::SampleCache sample_cache_;
};

} // namespace glaxnimate::model
