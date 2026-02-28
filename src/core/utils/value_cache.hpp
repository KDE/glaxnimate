/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <tuple>

namespace glaxnimate::util {

template<class T>
class ValueCache
{
public:
    bool is_dirty() const
    {
        return dirty;
    }

    bool is_clean() const
    {
        return !dirty;
    }

    void mark_dirty() { dirty = true; }

    const T& value() const { return cached_value; }

    void set_value(const T& value)
    {
        dirty = false;
        cached_value = value;
    }

public:
    bool dirty = true;
    T cached_value = {};
};


template<class... T>
class MultiValueCache
{
    void mark_dirty()
    {
        mark_dirty_impl(std::index_sequence_for<T...>{});
    }

    template<class U>
    auto& get() { return std::get<U>(caches); }

    template<class U>
    const auto& get() const { return std::get<U>(caches); }

private:
    template<std::size_t... I>
    void mark_dirty_impl(std::index_sequence<I...>)
    {
        (std::get<I>(caches).mark_dirty(), ...);
    }

    std::tuple<ValueCache<T>...> caches;
};

} // glaxnimate::util
