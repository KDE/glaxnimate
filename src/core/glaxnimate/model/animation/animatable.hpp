/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <limits>

#include <QList>

#include "glaxnimate/model/property/property.hpp"
#include "glaxnimate/math/math.hpp"
#include "glaxnimate/math/bezier/point.hpp"
#include "glaxnimate/math/bezier/solver.hpp"
#include "glaxnimate/math/bezier/bezier_length.hpp"
#include "glaxnimate/model/animation/animatable_base.hpp"

/*
 * Arguments: type, name, default, emitter, flags
 * For float: type, name, default, emitter, min, max, flags
 */
#define GLAXNIMATE_ANIMATABLE(type, name, ...)                                  \
public:                                                                         \
    glaxnimate::model::AnimatedProperty<type> name{this, kli18n(#name), __VA_ARGS__};   \
    glaxnimate::model::AnimatableBase* get_##name() { return &name; }           \
private:                                                                        \
    Q_PROPERTY(glaxnimate::model::AnimatableBase* name READ get_##name)         \
    Q_CLASSINFO(#name, "property animated " #type)                              \
    // macro end


namespace glaxnimate::model {

class AnimatedPropertyBase : public AnimatableBase, public BaseProperty
{
    Q_OBJECT

    Q_PROPERTY(QVariant value READ value WRITE set_undoable)

public:
    AnimatedPropertyBase(Object* object, const utils::LazyLocalizedString& name, PropertyTraits traits);

    int animatable_flags() const override
    {
        return HasValue|IsWritable|IsProperty;
    }

    bool set_undoable(const QVariant& val, bool commit=true) override;

    using BaseProperty::value;

    bool assign_from(const BaseProperty* prop) override;

    /**
     * \brief Set the current time
     * \post value() == value(time)
     */
    void set_time(FrameTime time) override
    {
        current_time = time;
        on_set_time(time);
    }

    FrameTime time() const
    {
        return current_time;
    }

    QVariant static_value() const override { return value(); }
    bool set_static_value(const QVariant& v) override { return set_value(v); }

    QVariant value(FrameTime time) const { return value_at_time(time); }
    QVariant value() const override = 0;
    QString visual_name() const override { return name(); }

    /**
     * \brief Removes all keyframes
     * \post !animated()
     */
    virtual void clear_keyframes() = 0;

    /**
     * \brief Removes the keyframe with the given time
     * \returns whether a keyframe was found and removed
     */
    virtual bool remove_keyframe_at_time(FrameTime time) = 0;

    /**
     * @brief Sets the transition at the given keyframe
     * @param time
     * @param transition
     */
    virtual void set_transition(FrameTime time, const KeyframeTransition& transition) = 0;

    /**
     * @brief Sets the transition at the keyframe before \p time
     * @param time
     * @param transition
     */
    virtual void set_transition_before(FrameTime time, const KeyframeTransition& transition) = 0;

    /**
     * \brief Moves a keyframe
     * \param from_time Time of the keyframe to move
     * \param to_time New time for the keyframe
     */
    virtual MoveResult move_keyframe(FrameTime from_time, FrameTime to_time) = 0;

    /**
     * \brief Sets a value at a keyframe
     * \param time          Time to set the value at
     * \param value         Value to set
     * \param info          If not nullptr, it will be written to with information about what has been node
     * \param force_insert  If \b true, it will always add a new keyframe
     * \post value(time) == \p value && animate() == true
     * \return The keyframe or nullptr if it couldn't be added.
     * If there is already a keyframe at \p time the returned value might be an existing keyframe
     */
    virtual KeyframeBase* set_keyframe(FrameTime time, const QVariant& value, SetKeyframeInfo* info = nullptr, bool force_insert = false) = 0;

    QUndoCommand* command_add_smooth_keyframe(FrameTime time, const QVariant& value, bool commit = true, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_remove_keyframe(FrameTime time, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_clear_keyframes(QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_set_transition(model::FrameTime time, const model::KeyframeTransition& transition, QUndoCommand* parent = nullptr) override;
    QUndoCommand* command_set_transition_side(
        model::FrameTime time,
        model::KeyframeTransition::Descriptive desc,
        const QPointF& point,
        bool before_transition,
        QUndoCommand* parent = nullptr
    ) override;
    QUndoCommand* command_move_keyframe(model::FrameTime time_before, model::FrameTime time_after, QUndoCommand* parent = nullptr) override;

protected:
    virtual void on_set_time(FrameTime time) = 0;

    FrameTime current_time = 0;
};

template<class Type>
class Keyframe : public KeyframeBase
{
public:
    using value_type = Type;
    using reference = const Type&;

    Keyframe(FrameTime time, Type value)
        : KeyframeBase(time), value_(std::move(value)) {}
    Keyframe(Keyframe&&) = default;

    void set(reference value)
    {
        value_ = value;
    }

    reference get() const
    {
        return value_;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
        {
            set(*v);
            return true;
        }
        return false;
    }

    value_type lerp(const Keyframe& other, double t) const
    {
        return math::lerp(value_, other.get(), this->transition().lerp_factor(t));
    }

    KeyframeTransition transition() const override { return transition_; }

    void set_transition(const KeyframeTransition& trans) override
    {
        transition_ = trans;
    }

protected:
    std::unique_ptr<KeyframeBase> do_clone() const override
    {
        auto clone = std::make_unique<Keyframe>(time(), value_);
        clone->set_transition(transition_);
        return clone;
    }

    class TypedKeyframeSplitter : public KeyframeSplitter
    {
    public:
        TypedKeyframeSplitter(const Keyframe* a, const Keyframe* b) : a(a), b(b) {}

        void step(const QPointF&) override {}

        std::unique_ptr<KeyframeBase> left(const QPointF& p) const override
        {
            return std::make_unique<Keyframe>(
                math::lerp(a->time(), b->time(), p.x()),
                math::lerp(a->get(), b->get(), p.y())
            );
        }

        std::unique_ptr<KeyframeBase> right(const QPointF& p) const override
        {
            return std::make_unique<Keyframe>(
                math::lerp(a->time(), b->time(), p.x()),
                math::lerp(a->get(), b->get(), p.y())
            );
        }

        std::unique_ptr<KeyframeBase> last() const override { return b->clone(); }

        const Keyframe* a;
        const Keyframe* b;
    };

    virtual std::unique_ptr<KeyframeSplitter> splitter(const KeyframeBase* other) const override
    {
        return std::make_unique<TypedKeyframeSplitter>(this, static_cast<const Keyframe*>(other));
    }

private:
    Type value_;
    KeyframeTransition transition_;
};


template<>
class Keyframe<QPointF> : public KeyframeBase
{
public:
    using value_type = QPointF;
    using reference = const QPointF&;

    Keyframe(FrameTime time, const QPointF& value)
        : KeyframeBase(time), point_(value) {}


    Keyframe(FrameTime time, const math::bezier::Point& value)
        : KeyframeBase(time), point_(value), linear(point_is_linear(value))
        {}

    Keyframe(Keyframe&&) = default;

    void set(reference value)
    {
        point_.translate_to(value);
    }

    reference get() const
    {
        return point_.pos;
    }

    QVariant value() const override
    {
        return QVariant::fromValue(point_);
    }

    bool set_value(const QVariant& val) override
    {
        if ( val.userType() == QMetaType::QPointF )
        {
            set(val.value<QPointF>());
            return true;
        }
        else if ( auto v = detail::variant_cast<math::bezier::Point>(val) )
        {
            set_point(*v);
            return true;
        }
        return false;
    }

    value_type lerp(const Keyframe& other, double t) const
    {
        auto factor = transition().lerp_factor(t);
        if ( linear && other.linear )
            return math::lerp(get(), other.get(), factor);

        auto solver = bezier_solver(other);
        math::bezier::LengthData len(solver, 20);
        return solver.solve(len.at_ratio(factor).ratio);
    }

    void set_point(const math::bezier::Point& point)
    {
        point_ = point;
        linear = point_is_linear(point);
    }

    const math::bezier::Point& point() const
    {
        return point_;
    }

    math::bezier::CubicBezierSolver<QPointF> bezier_solver(const Keyframe& other) const
    {
        return math::bezier::CubicBezierSolver<QPointF>(
            point_.pos, point_.tan_out, other.point_.tan_in, other.point_.pos
        );
    }

    bool is_linear() const
    {
        return linear;
    }

    KeyframeTransition transition() const override { return transition_; }

    void set_transition(const KeyframeTransition& trans) override
    {
        transition_ = trans;
    }

protected:
    std::unique_ptr<KeyframeBase> do_clone() const override
    {
        auto clone = std::make_unique<Keyframe>(time(), point_);
        clone->set_transition(transition_);
        return clone;
    }

    class PointKeyframeSplitter;
    std::unique_ptr<KeyframeSplitter> splitter(const KeyframeBase* other) const override;

private:
    static bool point_is_linear(const math::bezier::Point& point)
    {
        return point.tan_in == point.pos && point.tan_out == point.pos;
    }

    math::bezier::Point point_;
    bool linear = true;
    KeyframeTransition transition_;
};

template<class Type>
class AnimatedProperty;

namespace detail {

template<class Type>
class AnimatedProperty : public AnimatableImpl<Keyframe<Type>, AnimatedPropertyBase>
{
    using Base = AnimatableImpl<Keyframe<Type>, AnimatedPropertyBase>;
public:
    using keyframe_type = Keyframe<Type>;
    using value_type = typename Keyframe<Type>::value_type;
    using reference = typename Keyframe<Type>::reference;
    using iterator = typename KeyframeContainer<keyframe_type>::const_iterator;
    using mutable_iterator = typename KeyframeContainer<keyframe_type>::iterator;


    AnimatedProperty(
        Object* object,
        const utils::LazyLocalizedString& name,
        reference default_value,
        PropertyCallback<void, Type> emitter = {},
        int flags = 0
    )
    : Base(
        object, name, PropertyTraits::from_scalar<Type>(
            PropertyTraits::Animated|PropertyTraits::Visual|flags
        )),
      value_{default_value},
      emitter(std::move(emitter))
    {}


    QVariant value() const override
    {
        return QVariant::fromValue(value_);
    }

    QVariant value_at_time(FrameTime time) const override
    {
        return QVariant::fromValue(get_at(time));
    }

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val, AnimatedPropertyBase::SetKeyframeInfo* info = nullptr, bool force_insert = false) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return static_cast<model::AnimatedProperty<Type>*>(this)->set_keyframe(time, *v, info, force_insert);
        return nullptr;
    }

    void clear_keyframes() override
    {
        int n = this->keyframes_.size();
        this->keyframes_.clear();
        for ( int i = n - 1; i >= 0; i-- )
            Q_EMIT this->keyframe_removed(i);
    }

    bool remove_keyframe_at_time(FrameTime time) override
    {
        auto iter = this->keyframes_.find(time);
        if ( iter != this->keyframes_.end() )
        {
            auto prev = iter;
            bool update_prev = prev != this->keyframes_.begin();
            if ( update_prev )
                --prev;
            this->keyframes_.erase(iter);
            Q_EMIT this->keyframe_removed(time);
            if ( update_prev )
                on_keyframe_updated(prev);
            else
                this->value_changed();
            return true;
        }
        return false;
    }

    bool set_value(const QVariant& val) override
    {
        if ( auto v = detail::variant_cast<Type>(val) )
            return static_cast<model::AnimatedProperty<Type>*>(this)->set(*v);
        return false;
    }

    bool valid_value(const QVariant& val) const override
    {
        if ( detail::variant_cast<Type>(val) )
            return true;
        return false;
    }

    bool set(reference val)
    {
        value_ = val;
        mismatched_ = !this->keyframes_.empty();
        this->value_changed();
        emitter(this->object(), value_);
        return true;
    }

    keyframe_type* set_keyframe(FrameTime time, reference value, AnimatedPropertyBase::SetKeyframeInfo* info = nullptr, bool force_insert = false)
    {
        // First keyframe
        if ( this->keyframes_.empty() )
        {
            value_ = value;
            this->value_changed();
            emitter(this->object(), value_);
            auto it = this->keyframes_.insert(time, keyframe_type(time, value));
            Q_EMIT this->keyframe_added(time);
            if ( info )
                *info = {true};
            return it.ptr();
        }

        // Find the right keyframe
        auto kf = this->keyframes_.find_best(time);

        bool includes_current_time = this->keyframes_.affects_time(kf, this->time());

        // Time matches, update
        if ( kf->time() == time && !force_insert )
        {
            kf->set(value);
            if ( includes_current_time )
                this->set_time(this->time());
            Q_EMIT this->keyframe_updated(time);
            on_keyframe_updated(kf);
            if ( info )
                *info = {false};
            return kf.ptr();
        }

        // First keyframe not at 0, might have to add the new keyframe at 0
        if ( kf == this->keyframes_.begin() && kf->time() > time )
        {
            auto it = this->keyframes_.insert(kf, time, keyframe_type(time, value));
            if ( includes_current_time )
                this->set_time(this->time());
            Q_EMIT this->keyframe_added(time);
            on_keyframe_updated(it);
            if ( info )
                *info = {true};
            return it.ptr();
        }

        // Insert somewhere in the middle
        auto it = this->keyframes_.insert(kf, time, keyframe_type(time, value));
        if ( includes_current_time )
            this->set_time(this->time());
        Q_EMIT this->keyframe_added(time);
        on_keyframe_updated(it);
        if ( info )
            *info = {true};
        return it.ptr();
    }

    void set_transition(FrameTime time, const KeyframeTransition& transition) override
    {
        auto kf = this->keyframes_.find(time);
        if ( kf != this->keyframes_.end() )
        {
            kf->set_transition(transition);
            if ( this->keyframes_.affects_time(kf, this->time()) )
                this->set_time(this->time());
            Q_EMIT this->transition_changed(time, transition.before_descriptive(), transition.after_descriptive());
        }
    }

    void set_transition_before(FrameTime time, const KeyframeTransition& transition) override
    {
        auto kf = this->keyframes_.find_best(time);
        if ( kf == this->keyframes_.end() || kf == this->keyframes_.begin())
            return;
        --kf;

        kf->set_transition(transition);
        if ( this->keyframes_.affects_time(kf, time) )
            this->set_time(this->time());
        Q_EMIT this->transition_changed(time, transition.before_descriptive(), transition.after_descriptive());
    }

    value_type get() const
    {
        return value_;
    }

    value_type get_at(FrameTime time) const
    {
        if ( time == this->time() )
            return value_;
        return get_at_impl(time);
    }

    bool value_mismatch() const override
    {
        return mismatched_;
    }

    AnimatableBase::MoveResult move_keyframe(FrameTime from_time, FrameTime to_time) override
    {
        auto kf_at = this->keyframes_.find(from_time);
        if ( kf_at == this->keyframes_.end() )
            return AnimatableBase::MoveResult::NotFound;

        // Nothing to do
        if ( to_time == from_time )
            return AnimatableBase::MoveResult::Moved;

        bool includes_current_time = this->keyframes_.affects_time(kf_at, this->time());

        // Just move to a new position
        auto iter = this->keyframes_.lower_bound(to_time);
        if ( iter == this->keyframes_.end() || iter->time() != to_time )
        {
            kf_at->set_time(to_time);
            kf_at = this->keyframes_.move(kf_at, to_time);
            if ( includes_current_time || this->keyframes_.affects_time(kf_at, this->time()) )
                this->set_time(this->time());
            Q_EMIT this->keyframe_moved(from_time, to_time);
            return AnimatableBase::MoveResult::Moved;
        }

        // Needs to overwite
        iter->set_transition(kf_at->transition());
        iter->set(kf_at->get());
        this->keyframes_.erase(kf_at);
        if ( includes_current_time || this->keyframes_.affects_time(iter, this->time()) )
            this->set_time(this->time());
        Q_EMIT this->keyframe_removed(from_time);
        Q_EMIT this->keyframe_updated(to_time);
        return AnimatableBase::MoveResult::OverwrittenDestination;

        // why did I need all this?
        /*

            QPointF incoming(-1, -1);
            if ( keyframe_index > 0 )
            {
                auto trans_before_src = keyframes_[keyframe_index - 1]->transition();
                incoming = trans_before_src.after();
                trans_before_src.set_after(keyframes_[keyframe_index]->transition().after());
                keyframes_[keyframe_index - 1]->set_transition(trans_before_src);
            }


            auto move = std::move(keyframes_[keyframe_index]);
            keyframes_.erase(keyframes_.begin() + keyframe_index);
            keyframes_.insert(keyframes_.begin() + new_index, std::move(move));

            int ia = keyframe_index;
            int ib = new_index;
            if ( ia > ib )
                std::swap(ia, ib);

            if ( new_index > 0 )
            {
                auto trans_before_dst = keyframes_[new_index - 1]->transition();
                QPointF outgoing = trans_before_dst.after();

                if ( incoming.x() != -1 )
                {
                    trans_before_dst.set_after(incoming);
                    keyframes_[new_index - 1]->set_transition(trans_before_dst);
                }

                auto trans_moved = keyframes_[new_index]->transition();
                trans_moved.set_after(outgoing);
                keyframes_[new_index]->set_transition(trans_moved);
            }*/
    }

    SimpleRange<mutable_iterator> mutable_range() { return {
        this->keyframes_.begin(),
        this->keyframes_.end()
    }; }

    void stretch_time(qreal multiplier) override
    {
        std::vector<mutable_iterator> iters;
        iters.reserve(this->keyframes_.size());

        for ( auto it = this->keyframes_.begin(); it != this->keyframes_.end(); ++it )
            iters.push_back(it);

        // Ensure we never clash
        if ( multiplier > 1 )
            std::reverse(iters.begin(), iters.end());

        for ( auto& iter : iters )
        {
            auto from_time = iter->time();
            iter->stretch_time(multiplier);
            auto to_time = iter->time();
            this->keyframes_.move(iter, to_time);
            Q_EMIT this->keyframe_moved(from_time, to_time);
        }

        this->current_time *= multiplier;
    }

protected:
    void on_set_time(FrameTime time) override
    {
        if ( !this->keyframes_.empty() )
        {
            value_ = get_at_impl(time);
            this->value_changed();
            emitter(this->object(), value_);
        }
        mismatched_ = false;
    }

    void on_keyframe_updated(mutable_iterator kf)
    {
        auto cur_time = this->time();
        // if no keyframes or the current keyframe is being modified => update value_
        if ( !this->keyframes_.empty() && cur_time != kf->time() )
        {
            if ( kf->time() > cur_time )
            {
                // if the modified keyframe is far ahead => don't update value_
                if ( kf->time() > cur_time )
                    return;
            }
            else
            {
                ++kf;
                // if the modified keyframe is far behind => don't update value_
                if ( kf != this->keyframes_.end() && kf->time() < cur_time )
                    return;
            }
        }

        on_set_time(cur_time);
    }

    value_type get_at_impl(FrameTime time) const
    {
        auto iter_during = this->keyframes_.find_best(time);

        // No keyframe
        if ( iter_during == this->keyframes_.end() )
            return value_;

        // Before the first keyframe
        if ( time < iter_during->time() )
            return iter_during->get();

        auto iter_after = iter_during;
        ++iter_after;

        // After the last keyframe
        if ( iter_after == this->keyframes_.end() )
            return iter_during->get();

        // Interpolate between two keyframes
        double scaled_time = (time - iter_during->time()) / (iter_after->time() - iter_during->time());
        return iter_during->lerp(*iter_after, scaled_time);
    }

    QVariant do_mid_transition_value(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const override
    {
        return QVariant::fromValue(
            static_cast<const keyframe_type*>(kf_before)->lerp(
                *static_cast<const keyframe_type*>(kf_after),
                ratio
            )
        );
    }

    value_type value_;
    bool mismatched_ = false;
    PropertyCallback<void, Type> emitter;
};

// Intermediare non-templated class so Q_OBJECT works
class AnimatedPropertyPosition: public detail::AnimatedProperty<QPointF>
{
    Q_OBJECT
public:
    AnimatedPropertyPosition(
        Object* object,
        const utils::LazyLocalizedString& name,
        reference default_value,
        PropertyCallback<void, QPointF> emitter = {},
        int flags = 0
    ) : detail::AnimatedProperty<QPointF>(object, name, default_value, std::move(emitter), flags)
    {
    }


    void set_closed(bool closed);

    Q_INVOKABLE void split_segment(int index, qreal factor);

    Q_INVOKABLE bool set_bezier(glaxnimate::math::bezier::Bezier bezier);

    Q_INVOKABLE glaxnimate::math::bezier::Bezier bezier() const;

    void remove_points(const std::set<int>& indices);

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val, SetKeyframeInfo* info = nullptr, bool force_insert = false) override;

    keyframe_type* set_keyframe(FrameTime time, reference value, SetKeyframeInfo* info = nullptr, bool force_insert = false);

    bool set_value(const QVariant& val) override;

    bool valid_value(const QVariant& val) const override;

    QUndoCommand* command_add_smooth_keyframe(FrameTime time, const QVariant& value, bool commit = true, QUndoCommand* parent = nullptr) override;

    /**
     * \brief Gets the bezier derivative at the given time
     * \returns The derivative point, if defined
     */
    std::optional<QPointF> derivative_at(FrameTime time) const;

Q_SIGNALS:
    /// Invoked on set_bezier()
    void  bezier_set(const math::bezier::Bezier& bezier);
};


} // namespace detail


template<class Type>
class AnimatedProperty : public detail::AnimatedProperty<Type>
{
public:
    using detail::AnimatedProperty<Type>::AnimatedProperty;
};


template<>
class AnimatedProperty<float> : public detail::AnimatedProperty<float>
{
public:
    AnimatedProperty(
        Object* object,
        const utils::LazyLocalizedString& name,
        reference default_value,
        PropertyCallback<void, float> emitter = {},
        float min = std::numeric_limits<float>::lowest(),
        float max = std::numeric_limits<float>::max(),
        int flags = 0
    ) : detail::AnimatedProperty<float>(object, name, default_value, std::move(emitter), flags),
        min_(min),
        max_(max)
    {
    }

    float max() const { return max_; }
    float min() const { return min_; }

    bool set(reference val)
    {
        return detail::AnimatedProperty<float>::set(bound(val));
    }

    using AnimatedPropertyBase::set_keyframe;

    keyframe_type* set_keyframe(FrameTime time, reference value, SetKeyframeInfo* info = nullptr, bool force_insert = false)
    {
        return detail::AnimatedProperty<float>::set_keyframe(time, bound(value), info, force_insert);
    }

private:
    float bound(float value) const
    {
        return math::bound(min_, value, max_);
    }

    float min_;
    float max_;
};


template<>
class AnimatedProperty<QPointF> : public detail::AnimatedPropertyPosition
{
public:
    using detail::AnimatedPropertyPosition::AnimatedPropertyPosition;
};

} // namespace glaxnimate::model
