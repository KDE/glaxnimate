/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <utility>

namespace glaxnimate::utils {

/**
 * \brief Smart pointer that may or may not own the underlaying object
 */
template<class T>
class maybe_ptr
{
private:
    T* data;
    bool owns;

    void delete_owned() noexcept
    {
        if ( owns )
        {
            delete data;
            owns = false;
        }
    }

    void clear() noexcept
    {
        data = nullptr;
        owns = false;
    }
public:
    using pointer = T*;

    maybe_ptr(T* data, bool owns) noexcept : data(data), owns(owns) {}
    maybe_ptr() noexcept : data(nullptr), owns(false) {}
    maybe_ptr(const maybe_ptr&) = delete;
    maybe_ptr(maybe_ptr&& o) noexcept : data(o.data), owns(o.owns)
    {
        if ( o.owns )
            o.clear();
    }
    maybe_ptr& operator=(const maybe_ptr&) = delete;
    maybe_ptr& operator=(maybe_ptr&& o) noexcept
    {
        std::swap(o.data, data);
        std::swap(o.owns, owns);
        return *this;

    }

    ~maybe_ptr() noexcept { delete_owned(); }

    void reset()
    {
        delete_owned();
        data = nullptr;
    }

    void reset(T* data, bool owns)
    {
        delete_owned();
        this->data = data;
        this->owns = owns;
    }

    bool owns_pointer() const noexcept { return owns; }

    T* operator->() const noexcept { return data; }

    explicit operator bool() const noexcept { return data; }
};

} // glaxnimate::utils
