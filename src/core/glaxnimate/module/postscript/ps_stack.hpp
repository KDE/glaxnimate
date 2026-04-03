/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <deque>
#include "ps_value.hpp"

namespace glaxnimate::ps {

class Stack
{
public:
    using container = std::deque<Value>;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using value_type = container::value_type;
    using reference = container::const_reference;

    bool empty() const { return stack.empty(); }
    int size() const { return stack.size(); }

    void push(Value v)
    {
        stack.emplace_back(std::move(v));
    }

    Value pop()
    {
        Value v = stack.back();
        stack.pop_back();
        return v;
    }

    bool has(Value::Type type)
    {
        return !empty() && stack.back().type() == type;
    }

    template<Value::Type Tp>
    bool can_convert() const
    {
        return !empty() && stack.back().can_convert<Tp>();
    }

private:
    container stack;
};

} // namespace glaxnimate::ps

