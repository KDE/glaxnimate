/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <map>
#include <memory>
#include <iterator>

#include "glaxnimate/model/animation/frame_time.hpp"


namespace glaxnimate::model {

class KeyframeBase;


template<class Iterator>
class SimpleRange
{
public:
    using iterator = Iterator;

    SimpleRange(iterator begin_iter, iterator end_iter) :
        begin_iter(std::move(begin_iter)),
        end_iter(std::move(end_iter))
    {}

    iterator begin() const { return begin_iter; }
    iterator end() const { return end_iter; }

private:
    iterator begin_iter;
    iterator end_iter;
};

namespace detail {

class TypeErasedKeyframeIterator
{
public:
    using value_type = KeyframeBase;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = ptrdiff_t;

private:
    class VirtualIter
    {
    public:
        bool end = false;

        virtual ~VirtualIter() = default;
        virtual void increment() = 0;
        virtual void decrement() = 0;
        virtual pointer value() const = 0;
        virtual std::unique_ptr<VirtualIter> copy() const = 0;

        bool compare(const VirtualIter& oth) const
        {
            return ( end && oth.end) || oth.value() == value();
        }
    };

    template<class MapIter>
    class TemplateIter : public VirtualIter
    {
    public:
        MapIter iter;
        TemplateIter(MapIter iter, bool end) : iter(iter) { this->end = end; }

        void increment() override { ++iter; }
        void decrement() override { --iter; }
        pointer value() const override { return &iter->second; }
        std::unique_ptr<VirtualIter> copy() const override { return std::make_unique<TemplateIter>(iter, this->end); }

    };

    std::unique_ptr<VirtualIter> iter;

public:

    template<class MapIter>
    TypeErasedKeyframeIterator(MapIter iter, bool at_end) : iter(std::make_unique<TemplateIter<MapIter>>(iter, at_end)) {}
    TypeErasedKeyframeIterator(const TypeErasedKeyframeIterator& oth) : iter(oth.iter->copy()) {}
    TypeErasedKeyframeIterator& operator=(const TypeErasedKeyframeIterator& oth) { iter = oth.iter->copy(); return *this; }
    TypeErasedKeyframeIterator(TypeErasedKeyframeIterator&& oth) = default;
    TypeErasedKeyframeIterator& operator=(TypeErasedKeyframeIterator&& oth) = default;

    TypeErasedKeyframeIterator& operator++() noexcept { iter->increment(); return *this; }
    TypeErasedKeyframeIterator operator++(int) noexcept { auto copy = *this; ++*this; return copy; }
    TypeErasedKeyframeIterator& operator--() noexcept { iter->decrement(); return *this; }
    TypeErasedKeyframeIterator operator--(int) noexcept { auto copy = *this; --*this; return copy; }
    bool operator==(const TypeErasedKeyframeIterator& oth) const noexcept { return iter->compare(*oth.iter); }
    bool operator!=(const TypeErasedKeyframeIterator& oth) const noexcept { return !iter->compare(*oth.iter); }
    reference operator*() const { return *iter->value(); }
    pointer operator->() const { return iter->value(); }
};

using TypeErasedKeyframeRange = SimpleRange<TypeErasedKeyframeIterator>;

} // namespace detail

template<class T>
class KeyframeContainer
{
private:
    using container = std::map<FrameTime, T>;
    template<class InnerIter, class CVT>
    class iterator_type
    {
    public:
        using value_type = std::decay_t<T>;
        using reference = CVT&;
        using pointer = CVT*;
        using iterator_category = typename InnerIter::iterator_category;
        using difference_type = typename InnerIter::difference_type;

        iterator_type(InnerIter iter) noexcept : iter(iter) {}
        iterator_type& operator++() noexcept { ++iter; return *this; }
        iterator_type operator++(int) noexcept { auto copy = *this; ++*this; return copy; }
        iterator_type& operator--() noexcept { --iter; return *this; }
        iterator_type operator--(int) noexcept { auto copy = *this; --*this; return copy; }
        bool operator==(const iterator_type& oth) const noexcept { return iter == oth.iter; }
        bool operator!=(const iterator_type& oth) const noexcept { return iter != oth.iter; }
        reference operator*() const { return iter->second; }
        pointer operator->() const { return &iter->second; }
        pointer ptr() const { return &iter->second; }

        FrameTime key() const { return iter->first; }

        // Allow conversion from non-const to const iterator
        template<typename U = CVT, std::enable_if_t<std::is_const_v<U>, int> = 0>
        iterator_type(
            const iterator_type<typename container::iterator, value_type>& mutable_iterator
        ) noexcept : iter(mutable_iterator.iter) {}

    private:
        friend KeyframeContainer;
        InnerIter iter;
        // friend detail::TypeErasedKeyframeIterator::TemplateIter<T>;
    };

public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using iterator = iterator_type<typename container::iterator, T>;
    // TODO use std::basic_const_iterator?
    using const_iterator = iterator_type<typename container::const_iterator, const T>;
    using size_type = int;

    /**
     * \brief Returns iterator to the value before or equal to the given time
     * If all items are after \p time, it will return an iterator to the first item
     */
    iterator find_best(FrameTime time)
    {
        auto it = map.lower_bound(time);
        if ( it != map.begin() && (it == map.end() || it->first > time) )
            --it;
        return it;
    }

    const_iterator find_best(FrameTime time) const
    {
        auto it = map.lower_bound(time);
        if ( it != map.begin() )
            --it;
        return it;
    }

    /**
     * \brief Finds a keyframe from its actual time
     */
    iterator find(FrameTime time)
    {
        return map.find(time);
    }

    /**
     * \brief Finds a keyframe from its actual time
     */
    const_iterator find(FrameTime time) const
    {
        return map.find(time);
    }

    /**
     * \brief Iterator whose time is not less than \p time
     */
    iterator lower_bound(FrameTime time)
    {
        return map.lower_bound(time);
    }

    /**
     * \brief Iterator whose time is not less than \p time
     */
    const_iterator lower_bound(FrameTime time) const
    {
        return map.lower_bound(time);
    }

    /**
     * \brief Iterator whose time is greater than \p time
     */
    iterator upper_bound(FrameTime time)
    {
        return map.upper_bound(time);
    }

    /**
     * \brief Iterator whose time is greater than \p time
     */
    const_iterator upper_bound(FrameTime time) const
    {
        return map.upper_bound(time);
    }

    bool empty() const { return map.empty(); }
    size_type size() const { return map.size(); }

    iterator insert(FrameTime time, value_type t)
    {
        return map.emplace(time, std::move(t)).first;
    }

    iterator insert(iterator hint, FrameTime time, value_type t)
    {
        return map.emplace_hint(hint.iter, time, std::move(t));
    }

    iterator begin() { return map.begin(); }
    iterator end() { return map.end(); }
    const_iterator begin() const { return map.begin(); }
    const_iterator end() const { return map.end(); }
    const_iterator cbegin() const { return map.begin(); }
    const_iterator cend() const { return map.end(); }

    detail::TypeErasedKeyframeRange type_erased() const { return {
        detail::TypeErasedKeyframeIterator(map.begin(), false),
        detail::TypeErasedKeyframeIterator(map.end(), true)
    }; }

    detail::TypeErasedKeyframeIterator type_erased(const const_iterator& iter) const
    {
        return detail::TypeErasedKeyframeIterator(iter.iter, iter.iter == map.end());
    }

    /**
     * Updates the given iterator to have the given key
     * \pre no other element at \p dest
     */
    iterator move(iterator it, FrameTime dest)
    {
        auto node = map.extract(it.iter);
        node.key() = dest;
        return map.insert(std::move(node)).position;
    }

    iterator erase(iterator it) { return map.erase(it.iter); }
    void clear() { map.clear(); }

    /**
     * \brief Whether the iterator points to a segment that contains the given time
     * meadning find_best(time) will return it
     */
    bool contains_time(const_iterator it, FrameTime time) const
    {
        if ( it.iter == map.end() )
            return false;
        if ( it.key() > time )
            return it.iter == map.begin();
        ++it;
        if ( it.iter == map.end() )
            return true;
        return it.key() <= time;
    }

private:
    std::map<FrameTime, T> map;
};

} // namespace glaxnimate::model
