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
#include "glaxnimate/model/animation/keyframe_base.hpp"
#include "glaxnimate/model/animation/keyframe_container.hpp"

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

class AnimatableBase : public QObject, public BaseProperty
{
    Q_OBJECT

    Q_PROPERTY(int keyframe_count READ keyframe_count)
    Q_PROPERTY(QVariant value READ value WRITE set_undoable)
    Q_PROPERTY(bool animated READ animated)

public:
    enum KeyframeStatus
    {
        NotAnimated,    ///< Value is not animated
        Tween,          ///< Value is animated but the given time isn't a keyframe
        IsKeyframe,     ///< Value is animated and the given time is a keyframe
        Mismatch        ///< Value is animated and the current value doesn't match the animated value
    };

    struct SetKeyframeInfo
    {
        bool insertion;
    };

    struct MidTransition
    {
        enum Type
        {
            Invalid,
            SingleKeyframe,
            Middle,
        };
        Type type = Invalid;

        QVariant value;
        model::KeyframeTransition from_previous;
        model::KeyframeTransition to_next;
    };

    using BaseProperty::BaseProperty;

    virtual ~AnimatableBase() = default;

    /**
     * \brief Number of keyframes
     */
    virtual int keyframe_count() const = 0;

    /**
     * \brief Keyframe whose transition contains \p time
     *
     *  This keyframe starts before or at \p time and it ends at \p time
     *
     * If all keyframes are after \p time, returns first one
     * This means keyframe(keyframe_index(t)) is always valid when animated
     *
     *          keyframe_containing
     *          v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     * \return the Corresponding keyframe or nullptr if not found
     */
    Q_INVOKABLE virtual const KeyframeBase* keyframe_containing(FrameTime time) const = 0;
    virtual KeyframeBase* keyframe_containing(FrameTime time) = 0;
    /**
     * \brief Returns the keyframe fully before the given time
     *
     *
     *  keyframe_before
     *  v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     */
    Q_INVOKABLE virtual KeyframeBase* keyframe_before(FrameTime time) = 0;
    virtual const KeyframeBase* keyframe_before(FrameTime time) const = 0;

    /**
     * \brief Returns the keyframe fully before the given time
     *
     *
     *                  keyframe_before
     *                  v-------v
     *  t0      t1      t2      t3
     *  |-------|-------|-------|
     *          ^   ^
     *            time
     *
     */
    Q_INVOKABLE virtual KeyframeBase* keyframe_after(FrameTime time) = 0;
    virtual const KeyframeBase* keyframe_after(FrameTime time) const = 0;

    /**
     * \brief Keyframe at that specific time
     * \return the Corresponding keyframe or nullptr if not found
     */
    Q_INVOKABLE virtual const KeyframeBase* keyframe_at(FrameTime time) const = 0;
    virtual KeyframeBase* keyframe_at(FrameTime time) = 0;


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
     * \brief Get the value at the given time
     */
    virtual QVariant value(FrameTime time) const = 0;

    bool set_undoable(const QVariant& val, bool commit=true) override;

    using BaseProperty::value;

    enum class MoveResult
    {
        NotFound,
        OverwrittenDestination,
        Moved,
    };

    /**
     * \brief Moves a keyframe
     * \param from_time Time of the keyframe to move
     * \param to_time New time for the keyframe
     */
    virtual MoveResult move_keyframe(FrameTime from_time, FrameTime to_time) = 0;

    /**
     * If animated(), whether the current value has been changed over the animated value
     */
    Q_INVOKABLE virtual bool value_mismatch() const = 0;

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

    /**
     * \brief Whether it has multiple keyframes
     */
    bool animated() const
    {
        return keyframe_count() != 0;
    }

    KeyframeStatus keyframe_status(FrameTime time) const
    {
        if ( !animated() )
            return NotAnimated;
        if ( value_mismatch() )
            return Mismatch;
        if ( keyframe_at(time) )
            return IsKeyframe;
        return Tween;
    }

    bool has_keyframe(FrameTime time) const
    {
        if ( !animated() )
            return false;
        return keyframe_at(time);
    }

    MidTransition mid_transition(FrameTime time) const;

    /**
     * \brief Clears all keyframes and creates an associated undo action
     * \param value Value to be set after clearing
     */
    virtual void clear_keyframes_undoable(QVariant value = {});

    /**
     * \brief Adds a keyframe at the given time
     */
    virtual void add_smooth_keyframe_undoable(FrameTime time, const QVariant& value);

    virtual detail::TypeErasedKeyframeRange keyframe_range() const = 0;
    virtual detail::TypeErasedKeyframeIterator find(model::FrameTime t) const = 0;

    virtual const KeyframeBase* first_keyframe() const = 0;
    virtual const KeyframeBase* last_keyframe() const = 0;

Q_SIGNALS:
    void keyframe_added(FrameTime time);
    void keyframe_removed(FrameTime time);
    void keyframe_updated(FrameTime time);
    void keyframe_moved(FrameTime from_time, FrameTime to_time);

protected:
    virtual void on_set_time(FrameTime time) = 0;

    MidTransition do_mid_transition(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const;
    virtual QVariant do_mid_transition_value(const KeyframeBase* kf_before, const KeyframeBase* kf_after, qreal ratio) const = 0;

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
        Q_EMIT transition_changed(transition_.before_descriptive(), transition_.after_descriptive());
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
        Q_EMIT transition_changed(transition_.before_descriptive(), transition_.after_descriptive());
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

/**
 * @brief Implements common operations based on the keyframe container
 */
template<class KeyframeT, class Base>
class AnimatableImpl : public Base
{
public:
    using Ctor = AnimatableImpl;
    using keyframe_type = KeyframeT;

    using Base::Base;

    int keyframe_count() const override
    {
        return keyframes_.size();
    }

    const keyframe_type* keyframe_containing(FrameTime time) const override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_containing(FrameTime time) override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_before(FrameTime time) override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() || it == keyframes_.begin())
            return nullptr;
        --it;
        return it.ptr();
    }


    keyframe_type* keyframe_after(FrameTime time) override
    {
        auto it = keyframes_.upper_bound(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }


    const keyframe_type* keyframe_before(FrameTime time) const override
    {
        auto it = keyframes_.find_best(time);
        if ( it == keyframes_.end() || it == keyframes_.begin())
            return nullptr;
        --it;
        return it.ptr();
    }


    const keyframe_type* keyframe_after(FrameTime time) const override
    {
        auto it = keyframes_.upper_bound(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    const keyframe_type* keyframe_at(FrameTime time) const override
    {
        auto it = keyframes_.find(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    keyframe_type* keyframe_at(FrameTime time) override
    {
        auto it = keyframes_.find(time);
        if ( it == keyframes_.end() )
            return nullptr;
        return it.ptr();
    }

    detail::TypeErasedKeyframeRange keyframe_range() const override
    {
        return keyframes_.type_erased();
    }

    detail::TypeErasedKeyframeIterator find(model::FrameTime t) const override
    {
        return keyframes_.type_erased(keyframes_.find(t));
    }

    virtual const KeyframeBase* first_keyframe() const override
    {
        return keyframes_.empty() ? nullptr : keyframes_.begin().ptr();
    }

    virtual const KeyframeBase* last_keyframe() const override
    {
        if ( keyframes_.empty() )
            return nullptr;
        auto iter = keyframes_.end();
        --iter;
        return iter.ptr();
    }

    typename KeyframeContainer<KeyframeT>::const_iterator begin() const { return this->keyframes_.begin(); }
    typename KeyframeContainer<KeyframeT>::const_iterator end() const { return this->keyframes_.end(); }

protected:
    KeyframeContainer<KeyframeT> keyframes_;
};

template<class Type>
class AnimatedProperty : public AnimatableImpl<Keyframe<Type>, AnimatableBase>
{
    using Base = AnimatableImpl<Keyframe<Type>, AnimatableBase>;
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

    QVariant value(FrameTime time) const override
    {
        return QVariant::fromValue(get_at(time));
    }

    keyframe_type* set_keyframe(FrameTime time, const QVariant& val, AnimatableBase::SetKeyframeInfo* info = nullptr, bool force_insert = false) override
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

    keyframe_type* set_keyframe(FrameTime time, reference value, AnimatableBase::SetKeyframeInfo* info = nullptr, bool force_insert = false)
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

        bool includes_current_time = this->keyframes_.contains_time(kf, time);
        if ( time > this->time() && !includes_current_time )
        {
            auto prev = kf;
            --prev;
            includes_current_time = this->keyframes_.contains_time(prev, time);
        }

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
        Q_EMIT this->keyframe_added(time);
        if ( includes_current_time )
            this->set_time(this->time());
        on_keyframe_updated(it);
        if ( info )
            *info = {true};
        return it.ptr();
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

        // Just move to a new position
        auto iter = this->keyframes_.lower_bound(to_time);
        if ( iter == this->keyframes_.end() || iter->time() != to_time )
        {
            kf_at->set_time(to_time);
            this->keyframes_.move(kf_at, to_time);
            Q_EMIT this->keyframe_moved(from_time, to_time);
            return AnimatableBase::MoveResult::Moved;
        }

        // Needs to overwite
        iter->set_transition(kf_at->transition());
        iter->set(kf_at->get());
        this->keyframes_.erase(kf_at);
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

    void add_smooth_keyframe_undoable(FrameTime time, const QVariant& value) override;

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

    using AnimatableBase::set_keyframe;

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
